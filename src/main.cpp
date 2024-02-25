/*
 *   mash16 - the chip16 emulator
 *   Copyright (C) 2012-2013 tykel
 *
 *   mash16 is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   mash16 is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with mash16.  If not, see <http://www.gnu.org/licenses/>.
 */

/* A global used in other files. */
int use_verbose;

#include "options.h"
#include "consts.h"
#include "strings.h"
#include "header/header.h"
#include "core/gpu.h"
#include "core/audio.h"
#include "core/cpu.h"

#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl2.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <cassert>
#include <signal.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <algorithm>
#include <format>
#include <map>
#include <string>

#pragma pack(push,1)
typedef struct sym_entry
{
    uint16_t addr;
    uint16_t str_offs;
} sym_entry_t;
#pragma pack(pop)

struct watch_entry
{
    const char* symbol;
    bool enabled;
};

/* Globals used within the file. */
static program_opts opts;
static cpu_state* state;
static char *symbol_strs;
static char *symbols[0x10000];
static std::map<uint16_t, watch_entry> watches;
static std::map<uint16_t, watch_entry> breakps;
static SDL_Renderer *renderer;
static SDL_Window *window;
static GLuint screen;
static char strfps[256];

static bool show_debugger = false;

void (*cpu_exec)(cpu_state* state);

/* Timing variables. */
static int t = 0, oldt = 0;
static int fps = 0, lastsec = 0;
static int stop = 0;

static bool paused = false;

void pause_cpu(void)
{
   paused = true;
}

/* State printing options. */
static int hex = 1;

static std::string instr_string(cpu_state* state, uint16_t pc)
{
    instr ni {};
    ni.dword = * reinterpret_cast<uint32_t *>(&state->m[pc]);
    instr_type type = cpu_op_type(i_op(ni));
    const char *sym_imm = symbols[i_hhll(ni)];
    const char *sym_imm_pt = sym_imm ? sym_imm : "";
    const char sym_lp = sym_imm ? '(' : ' ';
    const char sym_rp = sym_imm ? ')' : ' ';
    std::string buf;

    buf.append(std::format("{}{} ", str_ops[i_op(ni)],
            i_op(ni)==0x12 || i_op(ni)==0x17 ? str_cond[i_yx(ni)&0xf]:""));
    switch(type)
    {
        case OP_HHLL:
            buf.append(std::format("${:04x}",i_hhll(ni)));
            buf.append(std::format(" {}{}{}", sym_lp, sym_imm_pt, sym_rp));
            break;
        case OP_N:
            buf.append(std::format("{:x}",i_n(ni)));
            break;
        case OP_R:
            buf.append(std::format("r{:x}",i_yx(ni)&0xf));
            break;
        case OP_R_N:
            buf.append(std::format("r{:x}, {:x}",i_yx(ni)&0xf,i_n(ni)));
            break;
        case OP_R_R:
            buf.append(std::format("r{:x}, r{:x}",i_yx(ni)&0xf, i_yx(ni) >> 4));
            break;
        case OP_R_R_R:
            buf.append(std::format("r{:x}, r{:x}, r{:x}",i_yx(ni)&0xf, i_yx(ni) >> 4, i_z(ni)));
            break;
        case OP_N_N:
            buf.append(std::format("{:x}, {:x}",i_hhll(ni) >> 9, (i_yx(ni) >> 8)&1));
            break;
        case OP_R_HHLL:
            buf.append(std::format("r{:x}, ${:04x}",i_yx(ni)&0xf, i_hhll(ni)));
            buf.append(std::format(" {}{}{}", sym_lp, sym_imm_pt, sym_rp));
            break;
        case OP_R_R_HHLL:
            buf.append(std::format("r{:x}, r{:x}, ${:04x}",i_yx(ni)&0xf, i_yx(ni) >> 4, i_hhll(ni)));
            buf.append(std::format(" {}{}{}", sym_lp, sym_imm_pt, sym_rp));
            break;
        case OP_HHLL_HHLL:
            buf.append(std::format("${:02x}, ${:04x}",i_yx(ni),i_hhll(ni)));
            break;
        case OP_SP_HHLL:
            buf.append(std::format("sp, ${:04x}",i_hhll(ni)));
            break;
        case OP_NONE:
        default:
            break;
    }
    return buf;
}

void print_state(cpu_state* state, uint16_t pc)
{
    instr ni;
    ni.dword = *(uint32_t *)&state->m[pc];
    int i;
    auto istr = instr_string(state, pc);

    if (!opts.debug_stdout)
       return;

    printf("state @ cycle %ld:",state->meta.target_cycles);
    printf("   %04x [ %s ]      %c%s%c\n",
            pc, istr.c_str(),
            symbols[pc] ? '(' : ' ',
            symbols[pc] ? symbols[pc] : "",
            symbols[pc] ? ')' : ' ');
    printf("--------------------------------------------------------------\n");
    printf("| pc:   0x%04x     |    sp:  0x%04x     |    flags: %c%c%c%c     | \n",
        pc,state->sp,state->f.c?'C':'_',state->f.z?'Z':'_',state->f.o?'O':'_',state->f.n?'N':'_');
    printf("| spr: %3dx%3d     |    bg:     0x%x     |    instr: %02x%02x%02x%02x |\n",
        state->sw,state->sh,state->bgc,i_op(ni),i_yx(ni),i_z(ni),i_res(ni));
    printf("--------------------------------------------------------------\n");
    for(i=0; i<4; ++i)
    {
        if(hex)
            printf("| r%x: 0x%04x   |  r%x: 0x%04x   |  r%x: 0x%04x   |  r%x: 0x%04x |\n",
                   i,((uint16_t*)state->r)[i],
                   i+4,((uint16_t*)state->r)[i+4],
                   i+8,((uint16_t*)state->r)[i+8],
                   i+12,((uint16_t*)state->r)[i+12]);
        else
            printf("| r%x: % 6d   |  r%x: % 6d   |  r%x: % 6d   |  r%x: % 6d |\n",
                   i,state->r[i],i+4,state->r[i+4],i+8,state->r[i+8],i+12,state->r[i+12]);
    }
    printf("--------------------------------------------------------------\n");
}

char* get_symbol(uint16_t a)
{
   return symbols[a];
}

static int verify_header(uint8_t* bin, int len)
{
    ch16_header* header;
    uint8_t *data;
    
    header = (ch16_header*)bin;
    data = (uint8_t*)(bin + sizeof(ch16_header));
    if(read_header(header,len,data))
        return 1;
    return 0;
}

/* Return length of file if success; otherwise 0 */
static int read_file(char* fp, uint8_t* buf)
{
    int len, read;
    FILE* romf;
    
    romf = fopen(fp,"rb");
    if(romf == NULL)
        return 0;
    
    fseek(romf,0,SEEK_END);
    len = ftell(romf);
    fseek(romf,0,SEEK_SET);

    read = fread(buf,sizeof(uint8_t),len,romf);
    fclose(romf);
    
    return (read == len) ? len : 0;
}

/* Populate the symbol list. */
int read_symbols(char *fp)
{
    FILE *symf;
    uint32_t stroffs;
    size_t strblksz;
    int i;

    memset(symbols, 0, sizeof(symbols));

    symf = fopen(fp, "rb");
    if(symf == NULL)
    {
        fprintf(stderr, "error: could not open \"%s\"\n", fp);
        return 0;
    }

    fread(&stroffs, sizeof(stroffs), 1, symf);
    fseek(symf, 0, SEEK_END);
    strblksz = ftell(symf) - stroffs;
    symbol_strs = (char*)malloc(strblksz);
    fseek(symf, stroffs, SEEK_SET);
    fread(symbol_strs, strblksz, 1, symf);

    fseek(symf, sizeof(stroffs), SEEK_SET);
    if(use_verbose)
        printf("found string table offset: %d bytes\n", stroffs);
    for(i = 0; i < stroffs - sizeof(sym_entry_t); i += sizeof(sym_entry_t))
    {
        sym_entry_t sym;
        fread(&sym, sizeof(sym), 1, symf);
        symbols[sym.addr] = &symbol_strs[sym.str_offs];
        if(use_verbose)
            printf("> found symbol: %s -> 0x%04x\n",
                    symbols[sym.addr], sym.addr);
    }

    return 1;
}

int parse_breakpoints(program_opts* opts)
{
    int i;
   
    for (i = 0; i < opts->num_breakpoints; i++) {
        bool has_symbol = false;
        char *s = opts->breakpoints[i];
        int v = strtol(s, NULL, 0);
        /* If strtol returns 0 from a string which isn't some form of 0, it is
         * relatively safe to say it must be a symbol. */
        if(v == 0 &&
            strncmp(s, "0", 6) &&
            strncmp(s, "0x0", 6) &&
            strncmp(s, "0x00", 6) &&
            strncmp(s, "0x0000", 6))
        {
            int j;
            for(j = 0; j < 0x10000; j++)
            {
                if(symbols[j] && !strncmp(s, symbols[j], 100))
                {
                    v = j;
                    has_symbol = true;
                    break;
                }
            }
            if(j == 0x10000)
            {
                fprintf(stderr, "error: symbol \"%s\" not found in exports\n", s); 
                return 0;
            }
        }
        breakps[v] = { has_symbol ? s : nullptr, true };
    }
    
    for (i = 0; i < opts->num_watchpoints; i++) {
        bool has_symbol = false;
        char *s = opts->watchpoints[i];
        int v = strtol(s, NULL, 0);
        /* If strtol returns 0 from a string which isn't some form of 0, it is
         * relatively safe to say it must be a symbol. */
        if(v == 0 &&
            strncmp(s, "0", 6) &&
            strncmp(s, "0x0", 6) &&
            strncmp(s, "0x00", 6) &&
            strncmp(s, "0x0000", 6))
        {
            int j;
            for(j = 0; j < 0x10000; j++)
            {
                if(symbols[j] && !strncmp(s, symbols[j], 100))
                {
                    v = j;
                    has_symbol = true;
                    break;
                }
            }
            if(j == 0x10000)
            {
                fprintf(stderr, "error: symbol \"%s\" not found in exports\n", s); 
                return 0;
            }
        }
        watches[v] = { has_symbol ? s : nullptr, true };
    }
    return 1;
}

/* Sanitize the input. */
void sanitize_options(program_opts* opts)
{
    int input_errors = 0;
    
    if(opts->video_scaler <= 0 || opts->video_scaler > 4)
    {
        fprintf(stderr,"error: scaler %dx not supported\n",opts->video_scaler);
        ++input_errors;
    }
    if(opts->audio_sample_rate != 8000 && opts->audio_sample_rate != 11025 &&
       opts->audio_sample_rate != 22050 && opts->audio_sample_rate != 44100 &&
       opts->audio_sample_rate != 48000)
    {
        fprintf(stderr,"error: %dHz sample rate not supported\n",opts->audio_sample_rate);
        ++input_errors;
    }
    if(opts->audio_buffer_size < 128)
    {
        fprintf(stderr,"error: audio buffer size (%d B) too small\n",opts->audio_buffer_size);
        ++input_errors;
    }
    if(opts->audio_volume < 0  || opts->audio_volume > 255)
    {
        fprintf(stderr, "error: volume %d not valid (range is 0-255)\n",opts->audio_volume);
        ++input_errors;
    }
    if(opts->use_cpu_rec)
    {
        cpu_exec = cpu_rec_1bblk;
        printf("> using experimental recompiler core\n");
    }

    if(input_errors)
        exit(1);
}

void breakpoint_handle(cpu_state *state)
{
    int i;
    paused = opts.use_breakall;
    
    /* Stop at watch point if necessary. */
    auto is_store = false;
    auto hhll = 0u;
    switch (i_op(state->i)) {
    // STM rx, HHLL
    case 0x30:
        is_store = true;
        hhll = i_hhll(state->i);
        break;
    // STM rx, ry
    case 0x31:
        is_store = true;
        hhll = state->r[i_yx(state->i)>>4];
        break;
    // PUSH rx
    // PUSHF rx
    case 0xc0:
    case 0xc4:
        is_store = true;
        hhll = state->sp - 2;
        break;
    default:
        break;
    }
    if (is_store) {
        const auto& it = watches.find(state->pc);
        if (it != watches.cend() && it->second.enabled) {
            printf("> hit watchpoint @ 0x%04x: op %02x\n",
                   it->first, i_op(state->i));
            paused = true;
        }
    }
    
    if (!paused && !breakps.empty()) {
        const auto& it = breakps.find(state->pc);
        if (it != breakps.cend() && it->second.enabled) {
            printf("> hit breakpoint @ 0x%04x\n", it->first);
            paused = true;
        }
    }
}

static void draw_imgui(cpu_state *state)
{
    ImGui::Begin("CPU state"); 
   
    ImGui::BeginTable("OuterTable", 2);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    ImGui::BeginTable("SpecialRegTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame);
    ImGui::TableSetupColumn("Special Registers", ImGuiTableFlags_None);
    ImGui::TableHeadersRow();
    ImGui::TableNextRow();
    {
       ImGui::TableSetColumnIndex(0);
       ImGui::Text("pc: $%04x ", state->pc);
       ImGui::TableNextColumn();
       ImGui::Text("sp: $%04x ", state->sp);
       ImGui::TableNextColumn();
       ImGui::Text("f = [ %c %c %c %c ] ",
             state->f.c ? 'C' : '_', state->f.z ? 'Z' : '_',
             state->f.o ? 'O' : '_', state->f.n ? 'N' : '_');
       ImGui::TableNextRow();
       ImGui::TableSetColumnIndex(0);
       ImGui::Text("bgc: $%1x ", state->bgc);
       ImVec4 bgc_col( 
          state->pal_r[state->bgc] / 255.0,
          state->pal_g[state->bgc] / 255.0,
          state->pal_b[state->bgc] / 255.0,
          1.0
          );
       ImGui::SameLine();
       ImGui::ColorButton("bgcButton", bgc_col,
                          ImGuiColorEditFlags_NoAlpha | 
                          ImGuiColorEditFlags_NoPicker |
                          ImGuiColorEditFlags_NoSmallPreview |
                          ImGuiColorEditFlags_NoTooltip,
                          ImVec2(14.f, 14.f));
       ImGui::TableNextColumn();
       ImGui::Text("flip: %c%c", state->fx ? 'X' : ' ', state->fy ? 'Y' : ' ');
       ImGui::TableNextColumn();
       ImGui::Text("spr: % 2u x % 2u ", state->sw, state->sh);
    }
    ImGui::EndTable();

    ImGui::BeginTable("RegTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame);
    ImGui::TableSetupColumn("Registers", ImGuiTableFlags_None);
    ImGui::TableHeadersRow();
    for (auto i = 0; i < 16; i += 4) {
       ImGui::TableNextRow();
       ImGui::TableSetColumnIndex(i % 2);
       ImGui::Text("r%x: $%04x ", i, state->r[i]);
       ImGui::TableNextColumn();
       ImGui::Text("r%x: $%04x ", i+1, state->r[i+1]);
       ImGui::TableNextColumn();
       ImGui::Text("r%x: $%04x ", i+2, state->r[i+2]);
       ImGui::TableNextColumn();
       ImGui::Text("r%x: $%04x ", i+3, state->r[i+3]);
    }
    ImGui::EndTable();

    // Breakpoints + Watch
    ImGui::BeginTable("BPW", 2, ImGuiTableFlags_SizingStretchProp);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    // Breakpoints 
    ImGui::BeginTable("BreakpointsTable", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH);
    ImGui::TableSetupColumn("", ImGuiTableFlags_None);
    ImGui::TableSetupColumn("Breakpoints", ImGuiTableFlags_None);
    ImGui::TableHeadersRow();
    for (auto & [a, e] : breakps) {
       ImGui::TableNextRow();
       ImGui::TableSetColumnIndex(0);
       std::string label(std::string("##bp@") + std::to_string(a));
       ImGui::Checkbox(label.c_str(), reinterpret_cast<bool*>(&e.enabled));
       ImGui::TableNextColumn();
       ImGui::Text("$%04x [%s]", a, e.symbol);
    }
    ImGui::EndTable();

    if (ImGui::Button("Add breakpoint"))
       ImGui::OpenPopup("BreakpointsAdd");
    if (ImGui::BeginPopup("BreakpointsAdd", ImGuiWindowFlags_None)) {
       static int addr = 0;
       ImGui::InputInt("Address", &addr, 2, 2, ImGuiInputTextFlags_CharsHexadecimal);
       if (ImGui::Button("Add")) {
          ImGui::CloseCurrentPopup();
          breakps[addr] = { nullptr, true };
       }
       ImGui::EndPopup();
    }

    ImGui::TableNextColumn();
    // Watch
    ImGui::BeginTable("WatchTable", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH);
    ImGui::TableSetupColumn("", ImGuiTableFlags_None);
    ImGui::TableSetupColumn("Watch points", ImGuiTableFlags_None);
    ImGui::TableHeadersRow();
    for (auto& [addr, entry] : watches) {
       ImGui::TableNextRow();
       ImGui::TableSetColumnIndex(0);
       std::string label(std::string("##wp@") + std::to_string(addr));
       ImGui::Checkbox(label.c_str(), &entry.enabled);
       ImGui::TableNextColumn();
       ImGui::Text("$%04x [%s]", addr, entry.symbol);
    }
    ImGui::EndTable();

    if (ImGui::Button("Add Watch Point"))
       ImGui::OpenPopup("WatchAdd");
    if (ImGui::BeginPopup("WatchAdd", ImGuiWindowFlags_None)) {
       static int addr = 0;
       ImGui::InputInt("Address", &addr, 2, 2, ImGuiInputTextFlags_CharsHexadecimal);
       if (ImGui::Button("Add")) {
          ImGui::CloseCurrentPopup();
          watches[static_cast<uint16_t>(addr)] = { nullptr, true };
       }
       ImGui::EndPopup();
    }

    ImGui::EndTable(); // BPW

    ImGui::TableNextColumn();

    // Disassembly of instructions
    ImGui::Text("Disassembly:");
    ImGui::BeginTable("DisasmTable", 2, ImGuiTableFlags_SizingStretchProp);
    for (auto count = 0u, a = (unsigned int)std::clamp(state->pc - 24, 0, 0xfffc);
         count < 13 && a <= 0xfffc;
         ++count, a += 4) {
       ImGui::TableNextRow();
       ImGui::TableSetColumnIndex(0);

       auto istr = instr_string(state, a);
       ImGui::Text("%s $%04x: [%02x %02x %02x %02x] %s",
                   a == state->pc ? ">" : " ", a,
                   state->m[a], state->m[a+1], state->m[a+2], state->m[a+3],
                   istr.c_str());
    }
    ImGui::EndTable();

    ImGui::EndTable(); // OuterTable

    ImGui::End();
}

/* Emulation loop. */
void emulation_loop()
{ 
    int i;
    SDL_Event evt;
    GLenum e;
    
    ImGui_ImplOpenGL2_NewFrame();
    if (e = glGetError())
       fprintf(stderr, "ImGui_ImplOpenGL2_NewFrame caused GL error %d.\n", e);
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
   
    if (paused) {
        SDL_Delay(FRAME_DT);
        
        if (opts.debug_ui) {
            draw_imgui(state);
        }
        if (opts.debug_stdout) {
            print_state(state, state->pc);
        }
    } else {
        /* If using strict emulation, limit to 1M cycles / sec. */
        if(opts.use_cpu_limit)
        {
            while(!state->meta.wait_vblnk && state->meta.cycles < FRAME_CYCLES)
            {
                breakpoint_handle(state);
                if (paused)
                    break;
                cpu_exec(state);
            }
            /* Avoid hogging the CPU... */
            while((double)(t = SDL_GetTicks()) - oldt < FRAME_DT)
                SDL_Delay(1);
            oldt = t;
            ++fps;
        }
        /* Otherwise, max out in 1/60th sec. */
        else
        {
            while((t = SDL_GetTicks()) - oldt <= FRAME_DT )
            {
                for(i=0; i<600; ++i)
                {
                    cpu_exec(state);
                    /* Don't forget to count our frames! */
                    if(state->meta.wait_vblnk)
                    {
                        state->meta.wait_vblnk = 0;
                        ++fps;
                    }
                    else if(state->meta.cycles >= FRAME_CYCLES)
                    {
                        state->meta.cycles = 0;
                        ++fps;
                    }
                }
            }
            oldt = t;
        }
        /* Update the FPS counter after every second, or second's worth of frames. */
        if((fps >= 60 && opts.use_cpu_limit) || t > lastsec + 1000)
        {
            /* Update the caption. */
            sprintf(strfps,"mash16 (%d fps) - %s",fps,opts.filename);
            SDL_SetWindowTitle(window, strfps);
            /* Reset timing info. */
            lastsec = t;
            fps = 0;
        }
    }
        
    /* Handle input. */
    while(SDL_PollEvent(&evt))
    {
        ImGui_ImplSDL2_ProcessEvent(&evt);

        switch(evt.type)
        {
            case SDL_KEYDOWN:
                cpu_io_update(&evt.key,state);
                if(evt.key.keysym.sym == SDLK_SPACE)
                {
                    paused = !paused;
                }
                else if(evt.key.keysym.sym == SDLK_n && paused)
                {
                    cpu_exec(state);
                }
                else if(evt.key.keysym.sym == SDLK_h && paused)
                {
                    hex = !hex;
                    print_state(state, state->meta.old_pc);
                }
                else if(evt.key.keysym.sym == SDLK_ESCAPE)
                    stop = 1;
                break;
            case SDL_KEYUP:
                cpu_io_update(&evt.key,state);
                break;
            case SDL_QUIT:
                stop = 1;
                break;
            default:
                break;
        }
    }
    /* Draw. */
    blit_screen(screen,state,opts.video_scaler);

    if (paused) {
    }
    ImGui::Render();

    glViewport(0, 0, opts.video_scaler*320, opts.video_scaler*240);
    if (e = glGetError())
       fprintf(stderr, "ImGui_ImplOpenGL2_NewFrame caused GL error %d.\n", e);
    
    glClearColor(1.0, 0.0, 0.0, 1.0);
    if (e = glGetError())
       fprintf(stderr, "gClearColor caused GL error %d.\n", e);
    glClear(GL_COLOR_BUFFER_BIT);
    if (e = glGetError())
       fprintf(stderr, "glClear caused GL error %d.\n", e);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, screen);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 1.0);
    glVertex3f(-1.f,-1.f,0.0);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(1.f, -1.f, 0.0);
    glTexCoord2f(1.0, 0.0);
    glVertex3f(1.f, 1.f, 0.0);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(-1.f, 1.f, 0.0);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    if (e = glGetError())
       fprintf(stderr, "ImGui_ImplOpenGL2_RenderData caused GL error %d.\n", e);
    SDL_GL_SwapWindow(window);
    if (e = glGetError())
       fprintf(stderr, "SDL_GL_SwapWindow caused GL error %d.\n", e);

    /* Reset vblank flag. */
    state->meta.wait_vblnk = 0;
    state->meta.cycles = 0;
    state->meta.old_pc = state->pc;
}

int main(int argc, char* argv[])
{
    int i, len, use_header, sdl_flags, video_flags, ret;
    uint8_t *buf, *mem;

    /* Ensure STDIO goes to the terminal in Windows. */
#ifdef _WIN32
    freopen( "CON", "w", stdout );
    freopen( "CON", "w", stderr );
#endif

    /* Set up default options, then read them from the command line. */
    opts.filename = NULL;
    opts.pal_filename = NULL;
    opts.sym_filename = NULL;
    opts.use_audio = 1;
    opts.audio_sample_rate = AUDIO_RATE;
    opts.audio_buffer_size = AUDIO_SAMPLES;
    opts.audio_volume = 128;
    opts.use_verbose = 0;
    opts.video_scaler = 3;
    opts.use_fullscreen = 0;
    opts.use_cpu_limit = 1;
    opts.use_cpu_rec = 0;
    opts.num_breakpoints = 0;
    opts.use_breakall = 0;
    opts.rng_seed = time(NULL);
    opts.debug_ui = 1;
    opts.debug_stdout = 0;
    cpu_exec = cpu_step;

    options_parse(argc,argv,&opts);
    use_verbose = opts.use_verbose;
    
    sanitize_options(&opts);
    if(opts.sym_filename)
    {
        read_symbols(opts.sym_filename);
        if(use_verbose)
            printf("loaded symbols from %s.\n", opts.sym_filename);
    }

    if(!parse_breakpoints(&opts))
    {
        exit(1);
    }
    if(use_verbose)
    {
        printf("total breakpoints: %d\n",opts.num_breakpoints);
        for (const auto& [a, e] : breakps)
        {
            printf("> bp: %s%s0x%x\n",
                   e.symbol ? e.symbol : "", e.symbol ? " = " : "", a);
        }
        printf("total watchpoints: %d\n",opts.num_watchpoints);
        for (const auto& [a, e] : watches)
        {
            printf("> wp: %s%s0x%x\n",
                   e.symbol ? e.symbol : "", e.symbol ? " = " : "", a);
        }
    }

    /* Read our rom file into memory */
    buf = NULL;
    if(!(buf = (uint8_t *)calloc(MEM_SIZE+sizeof(ch16_header),1)))
    {
        fprintf(stderr,"error: calloc failed (buf)\n");
        exit(1);
    }
    len = read_file(opts.filename,buf);
    if(!len)
    {
        fprintf(stderr,"error: file could not be opened\n");
        exit(1);   
    }

    /* Check if a rom header is present; if so, verify it */
    use_header = 0;
    if((char)buf[0] == 'C' && (char)buf[1] == 'H' &&
       (char)buf[2] == '1' && (char)buf[3] == '6')
    {
        use_header = 1;
        if(!verify_header(buf,len))
        {
            fprintf(stderr,"error: header integrity check failed\n");
            exit(1);
        }
        if(opts.use_verbose)
        {
            ch16_header* h = (ch16_header*)buf;
            printf("header integrity check: ok\n");
            printf("> spec. version:  %d.%d\n",h->spec_ver>>4,h->spec_ver&0x0f);
            printf("> rom size:       %d B\n",h->rom_size);
            printf("> start address:  0x%04x\n",h->start_addr);
            printf("> crc32 checksum: 0x%08x\n",h->crc32_sum);
        }
    }
    else if(opts.use_verbose)
    {
        printf("header not found\n");
    }

    /* Get a buffer without header. */
    mem = NULL;
    if(!(mem = (uint8_t *)malloc(MEM_SIZE)))
    {
        fprintf(stderr,"error: malloc failed (mem)\n");
        exit(1);
    }
    memcpy(mem,(uint8_t*)(buf + use_header*sizeof(ch16_header)),
           len - use_header*sizeof(ch16_header));
    free(buf);

    /* Initialise SDL target. */
    sdl_flags = SDL_INIT_VIDEO | SDL_INIT_TIMER;
    if(opts.use_audio)
        sdl_flags |= SDL_INIT_AUDIO;
    if(SDL_Init(sdl_flags) < 0)
    {
        fprintf(stderr,"error: failed to initialise SDL: %s\n",SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);

#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    if (ret = SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1))
       fprintf(stderr, "SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) returned %d\n", ret);
    if (ret = SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24))
       fprintf(stderr, "SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) returned %d\n", ret);
    if (ret = SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8))
       fprintf(stderr, "SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) returned %d\n", ret);
    if (ret = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2))
       fprintf(stderr, "SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) returned %d\n", ret);
    if (ret = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2))
       fprintf(stderr, "SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) returned %d\n", ret);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("mash16", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          opts.video_scaler * 320, opts.video_scaler * 240, window_flags);
    if (window == nullptr)
    {
        fprintf(stderr,"error: failed to init. video mode (%d x %d x 32 bpp): %s\n",
                opts.video_scaler*320,opts.video_scaler*240,
                SDL_GetError());
        exit(1);
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (SDL_GL_MakeCurrent(window, gl_context))
       fprintf(stderr, "SDL_GL_MakeCurrent() failed\n");
    if (opts.use_verbose)
       printf("Got GL version: %s\n", (char*)glGetString(GL_VERSION));
    if (SDL_GL_SetSwapInterval(1)) // Enable vsync
       fprintf(stderr, "SDL_GL_SetSwapInterval() failed\n");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsLight;//Dark();

    // Setup Platform/Renderer backends
    GLenum e;
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    if (e = glGetError())
       fprintf(stderr, "ImGui_ImplSDL2_InitForOpenGL caused GL error %d.\n", e);
    ImGui_ImplOpenGL2_Init();
    if (e = glGetError())
       fprintf(stderr, "ImGui_ImplOpenGL2_Init caused GL error %d.\n", e);

    if(opts.use_verbose)
        printf("SDL initialised: %d x %d x %d bpp%s\n",
                opts.video_scaler*320,opts.video_scaler*240,32,
                opts.use_fullscreen?" (fullscreen)":"");

    sprintf(strfps,"mash16 - %s",opts.filename);
    SDL_SetWindowTitle(window,strfps);

    glGenTextures(1, &screen);

    /* Initialise the chip16 processor state. */
    cpu_init(&state,mem,&opts);
    audio_init(state,&opts);
    init_pal(state);
    if(opts.use_verbose)
        printf("chip16 state initialised\n\n");

    if(opts.pal_filename != NULL)
        if(!read_palette(opts.pal_filename, state->pal))
            fprintf(stderr,"error: palette in %s could not be read, potential corruption\n",opts.pal_filename);

    while(!stop)
        emulation_loop();

    /* Tidy up before exit. */
    audio_free();
    cpu_free(state);
    free(mem); 

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    glDeleteTextures(1, &screen);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    if(opts.use_verbose)
        printf("memory freed, goodbye\n");
    exit(0);
}

void panic(const char* format, ...)
{
   raise(SIGTRAP);
   
   va_list va {};
   va_start(va, format);
   vfprintf(stderr, format, va);
   va_end(va);

   exit(1);
}
