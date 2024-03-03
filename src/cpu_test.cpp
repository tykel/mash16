#include <signal.h>
#include <time.h>

#include "core/cpu.h"
#include "header/header.h"
#include "options.h"

int use_verbose = 1;

void panic(const char* format, ...)
{
   raise(SIGTRAP);
   
   va_list va = {};
   va_start(va, format);
   vfprintf(stderr, format, va);
   va_end(va);

   exit(1);
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

char *get_symbol(uint16_t a)
{
   return NULL;
}

void pause_cpu(void)
{
}

void print_state(cpu_state* state, uint16_t pc)
{
    int i;

    printf("state @ cycle %ld:\n",state->meta.target_cycles);
    printf("-------------------+--------------------+---------------------\n");
    printf("| pc:   0x%04x     |    sp:  0x%04x     |    flags: %c%c%c%c     | \n",
        pc,state->sp,state->f.c?'C':'_',state->f.z?'Z':'_',state->f.o?'O':'_',state->f.n?'N':'_');
    printf("| spr: %3dx%3d     |    bg:     0x%x     |    instr: %02x%02x%02x%02x |\n",
        state->sw,state->sh,state->bgc,i_op(state->i),i_yx(state->i),i_z(state->i),i_res(state->i));
    printf("+------------------+--------------------+--------------------+\n");
    for(i=0; i<4; ++i)
    {
            printf("| r%x: 0x%04x   |  r%x: 0x%04x   |  r%x: 0x%04x   |  r%x: 0x%04x |\n",
                   i,((uint16_t*)state->r)[i],
                   i+4,((uint16_t*)state->r)[i+4],
                   i+8,((uint16_t*)state->r)[i+8],
                   i+12,((uint16_t*)state->r)[i+12]);
    }
    printf("+------------------------------------------------------------+\n");
}

static size_t memdiffat(uint8_t *m1, uint8_t *m2, size_t max_size)
{
   size_t offs = 0;
   while (offs < max_size) {
      if (m1[offs] != m2[offs]) {
         break;
      }
      ++offs;
   }
   return offs;
}

int main(int argc, char **argv)
{
   uint8_t *buf = NULL, *mi = NULL, *mr = NULL;
   cpu_state *is, *rs;
   program_opts os;

   memset(&is, 0, sizeof(is));
   memset(&rs, 0, sizeof(rs));
   memset(&os, 0, sizeof(os));

   os.filename = argv[1];
   os.rng_seed = time(NULL);
   os.cpu_rec_1bblk_per_op = 1;

   /* Read our rom file into memory */
   buf = NULL;
   if(!(buf = (uint8_t *)calloc(MEM_SIZE+sizeof(ch16_header),1))) {
      fprintf(stderr,"error: calloc failed (buf)\n");
      exit(1);
   }
   size_t len = read_file(os.filename,buf);
   if(!len) {
      fprintf(stderr,"error: file could not be opened\n");
      exit(1);   
   }

   /* Check if a rom header is present; if so, verify it */
   int use_header = 0;
   if((char)buf[0] == 'C' && (char)buf[1] == 'H' &&
      (char)buf[2] == '1' && (char)buf[3] == '6') {
      use_header = 1;
      if(!verify_header(buf,len)) {
         fprintf(stderr,"error: header integrity check failed\n");
         exit(1);
      }
   }

   /* Get a buffer without header. */
   if(!(mi = (uint8_t *)malloc(MEM_SIZE))) {
      fprintf(stderr,"error: malloc failed (mem)\n");
      exit(1);
   }
   memset(mi, 0, MEM_SIZE);
   memcpy(mi,(uint8_t*)(buf + use_header*sizeof(ch16_header)),
          len - use_header*sizeof(ch16_header));
   
   if(!(mr = (uint8_t *)malloc(MEM_SIZE))) {
      fprintf(stderr,"error: malloc failed (mem)\n");
      exit(1);
   }
   memset(mr, 0, MEM_SIZE);
   memcpy(mr,(uint8_t*)(buf + use_header*sizeof(ch16_header)),
          len - use_header*sizeof(ch16_header));
  
   free(buf);

   cpu_init(&is, mi, &os);
   cpu_init(&rs, mr, &os);

   for (int cycles = 0; ; ++cycles) {
      uint16_t pc = is->pc;
      if (memcmp(is, rs, (uint8_t *)&is->m - (uint8_t*)is) != 0 ||
          memcmp(&is->bgc, &rs->bgc, (uint8_t*)&is->pal - (uint8_t*)&is->bgc) != 0) {
         printf("states differ:\n");
         printf("interpreter:\n");
         print_state(is, pc);
         printf("recompiler:\n");
         print_state(rs, pc);
         panic("raise SIGTRAP");
         break;
      }
      if (memcmp(is->m, rs->m, MEM_SIZE) != 0) {
         size_t offs = memdiffat(is->m, rs->m, MEM_SIZE);
         printf("memory differs @ offset %zu/0x%zx:\n", offs, offs);
         printf("interpreter: %02x\n", is->m[offs]);
         printf("recompiler: %02x\n", rs->m[offs]);
         printf("pc = 0x%04x\n", is->pc);
         panic("raise SIGTRAP");
         break;
      }
      if (memcmp(is->vm, rs->vm, 320*240) != 0) {
         size_t offs = memdiffat(is->m, rs->m, 320*240);
         printf("vm differs @ offset %zu/0x%zx:\n", offs, offs);
         printf("interpreter: %02x\n", is->vm[offs]);
         printf("recompiler: %02x\n", rs->vm[offs]);
         printf("pc = 0x%04x\n", is->pc);
         panic("raise SIGTRAP");
         break;
      }
      cpu_rec_1bblk(rs);
      {
         cpu_rec_bblk *bb = &rs->rec.bblk_map[pc];
         for (int cycles = bb->cycles; cycles > 0; --cycles) {
            cpu_step(is);
         }
      }
      
      if (cycles && ((cycles % 100000) == 0)) {
         printf("%d cycles elapsed. pc = %04x. r5 = %04x.\n", cycles, is->pc, *(uint16_t *)&is->r[5]);
      }
   }

   free(mi);
   free(mr);
}
