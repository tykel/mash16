#include "inspector.h"
#include "../strings.h"
#include <cstring>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <cctype>

namespace mash16 {

struct Inspector::EventSubscription {
    std::mutex mtx;
    std::condition_variable cv;
    std::deque<Event> q;
    bool closed = false;
};

Inspector::Inspector(cpu_state *cpu, std::recursive_mutex *cpu_mtx)
    : cpu_(cpu), cpu_mtx_(cpu_mtx ? cpu_mtx : &local_cpu_mtx_) {
    stop_state_.reason = "initialized";
    stop_state_.pc = cpu_ ? cpu_->pc : 0;
}

Inspector::Registers Inspector::getRegisters() {
    std::lock_guard<std::recursive_mutex> lk(*cpu_mtx_);
    Registers regs;
    for (int i = 0; i < 16; ++i) regs.r[i] = cpu_->r[i];
    regs.pc = cpu_->pc;
    regs.sp = cpu_->sp;
    regs.f = cpu_->f;
    return regs;
}

bool Inspector::writeRegister(const std::string& name, int32_t value) {
    std::lock_guard<std::recursive_mutex> lk(*cpu_mtx_);
    if (!cpu_) return false;
    if (name == "pc") {
        cpu_->pc = static_cast<uint16_t>(value);
        return true;
    }
    if (name == "sp") {
        cpu_->sp = static_cast<uint16_t>(value);
        return true;
    }
    if (name.size() >= 2 && (name[0] == 'r' || name[0] == 'R')) {
        char *end = nullptr;
        long idx = strtol(name.c_str() + 1, &end, 0);
        if (end && *end == '\0' && idx >= 0 && idx < 16) {
            cpu_->r[idx] = static_cast<int16_t>(value);
            return true;
        }
    }
    return false;
}

std::vector<uint8_t> Inspector::readMemory(uint16_t addr, size_t size) {
    std::lock_guard<std::recursive_mutex> lk(*cpu_mtx_);
    if (!cpu_ || !cpu_->m) return {};
    if (addr >= MEM_SIZE) return {};
    size_t avail = std::min<size_t>(size, MEM_SIZE - addr);
    return std::vector<uint8_t>(cpu_->m + addr, cpu_->m + addr + avail);
}

bool Inspector::writeMemory(uint16_t addr, const std::vector<uint8_t>& data) {
    std::lock_guard<std::recursive_mutex> lk(*cpu_mtx_);
    if (!cpu_ || !cpu_->m) return false;
    if (addr >= MEM_SIZE) return false;
    size_t avail = std::min<size_t>(data.size(), MEM_SIZE - addr);
    memcpy(cpu_->m + addr, data.data(), avail);
    cpu_rec_invalidate(cpu_, addr, avail);
    // Emit memory write event
    Event ev;
    ev.type = "mem_write";
    std::ostringstream o;
    o << "{\"type\":\"mem_write\",\"addr\":" << addr << ",\"bytes\": [";
    for (size_t i = 0; i < avail; ++i) { if (i) o << ","; o << (int)data[i]; }
    o << "]}";
    ev.payload = o.str();
    pushEvent(ev);
    std::vector<uint16_t> watches = listWatchpoints();
    for (uint16_t watch : watches) {
        if (watch >= addr && watch < addr + avail) {
            Event wev;
            wev.type = "watchpoint";
            std::ostringstream wo;
            wo << "{\"type\":\"watchpoint\",\"addr\":" << watch << "}";
            wev.payload = wo.str();
            pushEvent(wev);
            break;
        }
    }
    return true;
}

static std::optional<uint8_t> controller_button_mask(const std::string& button) {
    std::string name = button;
    std::transform(name.begin(), name.end(), name.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (name == "up") return PAD_UP;
    if (name == "down") return PAD_DOWN;
    if (name == "left") return PAD_LEFT;
    if (name == "right") return PAD_RIGHT;
    if (name == "select") return PAD_SELECT;
    if (name == "start") return PAD_START;
    if (name == "a") return PAD_A;
    if (name == "b") return PAD_B;
    return std::nullopt;
}

bool Inspector::setControllerButton(uint32_t player, const std::string& button, bool pressed) {
    auto mask = controller_button_mask(button);
    if (!mask || player < 1 || player > 2) return false;

    std::lock_guard<std::recursive_mutex> lk(*cpu_mtx_);
    if (!cpu_ || !cpu_->m) return false;

    uint16_t addr = player == 1 ? IO_PAD1_ADDR : IO_PAD2_ADDR;
    if (pressed) cpu_->m[addr] |= *mask;
    else cpu_->m[addr] &= static_cast<uint8_t>(~*mask);

    Event ev;
    ev.type = "controller";
    std::ostringstream o;
    o << "{\"type\":\"controller\",\"player\":" << static_cast<int>(player)
      << ",\"button\":\"" << button << "\",\"pressed\":"
      << (pressed ? "true" : "false")
      << ",\"state\":" << static_cast<int>(cpu_->m[addr]) << "}";
    ev.payload = o.str();
    pushEvent(ev);
    return true;
}

void Inspector::setBreakpoint(uint16_t addr) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (std::find(breakpoints_.begin(), breakpoints_.end(), addr) == breakpoints_.end())
        breakpoints_.push_back(addr);
}

void Inspector::clearBreakpoint(uint16_t addr) {
    std::lock_guard<std::mutex> lk(mtx_);
    breakpoints_.erase(std::remove(breakpoints_.begin(), breakpoints_.end(), addr), breakpoints_.end());
}

std::vector<uint16_t> Inspector::listBreakpoints() {
    std::lock_guard<std::mutex> lk(mtx_);
    return breakpoints_;
}

void Inspector::setWatchpoint(uint16_t addr) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (std::find(watchpoints_.begin(), watchpoints_.end(), addr) == watchpoints_.end())
        watchpoints_.push_back(addr);
}

void Inspector::clearWatchpoint(uint16_t addr) {
    std::lock_guard<std::mutex> lk(mtx_);
    watchpoints_.erase(std::remove(watchpoints_.begin(), watchpoints_.end(), addr), watchpoints_.end());
}

std::vector<uint16_t> Inspector::listWatchpoints() {
    std::lock_guard<std::mutex> lk(mtx_);
    return watchpoints_;
}

void Inspector::setSymbol(uint16_t addr, const std::string& name) {
    std::lock_guard<std::mutex> lk(mtx_);
    symbols_[addr] = name;
}

void Inspector::clearSymbols() {
    std::lock_guard<std::mutex> lk(mtx_);
    symbols_.clear();
}

std::map<uint16_t, std::string> Inspector::listSymbols() {
    std::lock_guard<std::mutex> lk(mtx_);
    return symbols_;
}

std::optional<uint16_t> Inspector::resolveSymbol(const std::string& name) {
    std::lock_guard<std::mutex> lk(mtx_);
    for (const auto& [addr, sym] : symbols_) {
        if (sym == name) return addr;
    }
    return std::nullopt;
}

std::optional<uint16_t> Inspector::resolveAddressOrSymbol(const std::string& text) {
    if (text == "pc") return getRegisters().pc;
    if (text == "sp") return getRegisters().sp;
    char *end = nullptr;
    unsigned long value = strtoul(text.c_str(), &end, 0);
    if (end && *end == '\0' && value <= 0xffff) return static_cast<uint16_t>(value);
    return resolveSymbol(text);
}

static std::string hex_word(uint16_t value) {
    std::ostringstream o;
    o << "0x" << std::hex << std::setw(4) << std::setfill('0') << value;
    return o.str();
}

static instr_type inspector_op_type(uint8_t op) {
    switch (op) {
        case 0x03: return OP_N;
        case 0x04: case 0x0a: case 0x0b: case 0x0c: case 0x10: case 0x11:
        case 0x12: case 0x14: case 0x17: case 0xd0:
            return OP_HHLL;
        case 0x05: case 0x13:
            return OP_R_R_HHLL;
        case 0x06:
            return OP_R_R_R;
        case 0x07: case 0x0d: case 0x20: case 0x22: case 0x30: case 0x40:
        case 0x50: case 0x53: case 0x60: case 0x63: case 0x70: case 0x80:
        case 0x90: case 0xa0: case 0xa3: case 0xa6: case 0xe0: case 0xe3:
            return OP_R_HHLL;
        case 0x08:
            return OP_N_N;
        case 0x16: case 0x18: case 0xc0: case 0xc1: case 0xd1: case 0xe1:
        case 0xe4:
            return OP_R;
        case 0x21:
            return OP_SP_HHLL;
        case 0x23: case 0x24: case 0x31: case 0x41: case 0x51: case 0x54:
        case 0x61: case 0x64: case 0x71: case 0x81: case 0x91: case 0xa1:
        case 0xa4: case 0xa7: case 0xb3: case 0xb4: case 0xb5: case 0xe2:
        case 0xe5:
            return OP_R_R;
        case 0x42: case 0x52: case 0x62: case 0x72: case 0x82: case 0x92:
        case 0xa2: case 0xa5: case 0xa8:
            return OP_R_R_R;
        case 0x0e:
            return OP_HHLL_HHLL;
        case 0xb0: case 0xb1: case 0xb2:
            return OP_R_N;
        default:
            return OP_NONE;
    }
}

Inspector::Instruction Inspector::disassembleOne(uint16_t addr) {
    std::lock_guard<std::recursive_mutex> lk(*cpu_mtx_);
    Instruction out;
    out.addr = addr;
    if (!cpu_ || !cpu_->m || addr > MEM_SIZE - 4) return out;

    instr ni {};
    ni.dword = static_cast<uint32_t>(cpu_->m[addr]) |
        (static_cast<uint32_t>(cpu_->m[addr + 1]) << 8) |
        (static_cast<uint32_t>(cpu_->m[addr + 2]) << 16) |
        (static_cast<uint32_t>(cpu_->m[addr + 3]) << 24);
    out.raw = ni.dword;
    out.op = i_op(ni);
    out.type = inspector_op_type(i_op(ni));
    out.mnemonic = str_ops[i_op(ni)] ? str_ops[i_op(ni)] : "";
    if (out.mnemonic == "___") out.mnemonic = "invalid";

    auto symbols = listSymbols();
    auto sym_it = symbols.find(addr);
    if (sym_it != symbols.end()) out.symbol = sym_it->second;
    auto imm_it = symbols.find(i_hhll(ni));

    std::ostringstream text;
    text << out.mnemonic;
    if (i_op(ni) == 0x12 || i_op(ni) == 0x17) {
        static const char *cond[16] = {
            "z","nz","n","nn","p","o","no","a","nc","c","be","g","ge","l","le","*"
        };
        text << cond[i_yx(ni) & 0xf];
    }
    switch(out.type) {
        case OP_HHLL:
            out.immediate = i_hhll(ni);
            text << " " << hex_word(i_hhll(ni));
            break;
        case OP_N:
            text << " " << std::hex << static_cast<int>(i_n(ni));
            break;
        case OP_R:
            text << " r" << std::hex << static_cast<int>(i_yx(ni) & 0xf);
            break;
        case OP_R_N:
            text << " r" << std::hex << static_cast<int>(i_yx(ni) & 0xf)
                 << ", " << static_cast<int>(i_n(ni));
            break;
        case OP_R_R:
            text << " r" << std::hex << static_cast<int>(i_yx(ni) & 0xf)
                 << ", r" << static_cast<int>(i_yx(ni) >> 4);
            break;
        case OP_R_R_R:
            text << " r" << std::hex << static_cast<int>(i_yx(ni) & 0xf)
                 << ", r" << static_cast<int>(i_yx(ni) >> 4)
                 << ", r" << static_cast<int>(i_z(ni));
            break;
        case OP_N_N:
            text << " " << std::hex << static_cast<int>(i_hhll(ni) >> 9)
                 << ", " << static_cast<int>((i_yx(ni) >> 8) & 1);
            break;
        case OP_R_HHLL:
            out.immediate = i_hhll(ni);
            text << " r" << std::hex << static_cast<int>(i_yx(ni) & 0xf)
                 << ", " << hex_word(i_hhll(ni));
            break;
        case OP_R_R_HHLL:
            out.immediate = i_hhll(ni);
            text << " r" << std::hex << static_cast<int>(i_yx(ni) & 0xf)
                 << ", r" << static_cast<int>(i_yx(ni) >> 4)
                 << ", " << hex_word(i_hhll(ni));
            break;
        case OP_HHLL_HHLL:
            out.immediate = i_hhll(ni);
            text << " " << hex_word(i_yx(ni)) << ", " << hex_word(i_hhll(ni));
            break;
        case OP_SP_HHLL:
            out.immediate = i_hhll(ni);
            text << " sp, " << hex_word(i_hhll(ni));
            break;
        case OP_NONE:
        default:
            break;
    }
    if (imm_it != symbols.end()) text << " (" << imm_it->second << ")";
    out.text = text.str();
    return out;
}

std::vector<Inspector::Instruction> Inspector::disassemble(uint16_t addr, size_t count) {
    std::vector<Instruction> out;
    out.reserve(count);
    for (size_t i = 0; i < count && addr <= MEM_SIZE - 4; ++i, addr += 4)
        out.push_back(disassembleOne(addr));
    return out;
}

Inspector::StopState Inspector::stopState() {
    std::lock_guard<std::mutex> lk(stop_mtx_);
    return stop_state_;
}

bool Inspector::waitStopped(StopState& out, int timeout_ms, uint64_t after_sequence) {
    std::unique_lock<std::mutex> lk(stop_mtx_);
    auto pred = [&] { return stop_state_.sequence > after_sequence; };
    if (!pred()) {
        if (timeout_ms <= 0) return false;
        if (!stop_cv_.wait_for(lk, std::chrono::milliseconds(timeout_ms), pred)) {
            return false;
        }
    }
    out = stop_state_;
    return true;
}

std::vector<uint8_t> Inspector::snapshot() {
    std::lock_guard<std::recursive_mutex> lk(*cpu_mtx_);
    std::vector<uint8_t> out;
    out.reserve(sizeof(cpu_->r) + sizeof(cpu_->pc) + sizeof(cpu_->sp) + sizeof(cpu_->f) + MEM_SIZE);

    // copy registers
    out.insert(out.end(), reinterpret_cast<uint8_t*>(&cpu_->r[0]), reinterpret_cast<uint8_t*>(&cpu_->r[0]) + sizeof(cpu_->r));
    out.insert(out.end(), reinterpret_cast<uint8_t*>(&cpu_->pc), reinterpret_cast<uint8_t*>(&cpu_->pc) + sizeof(cpu_->pc));
    out.insert(out.end(), reinterpret_cast<uint8_t*>(&cpu_->sp), reinterpret_cast<uint8_t*>(&cpu_->sp) + sizeof(cpu_->sp));
    out.insert(out.end(), reinterpret_cast<uint8_t*>(&cpu_->f), reinterpret_cast<uint8_t*>(&cpu_->f) + sizeof(cpu_->f));

    // memory
    if (cpu_->m)
        out.insert(out.end(), cpu_->m, cpu_->m + MEM_SIZE);

    return out;
}

bool Inspector::restore(const std::vector<uint8_t>& data) {
    std::lock_guard<std::recursive_mutex> lk(*cpu_mtx_);
    size_t expected = sizeof(cpu_->r) + sizeof(cpu_->pc) + sizeof(cpu_->sp) + sizeof(cpu_->f) + MEM_SIZE;
    if (data.size() < expected) return false;
    size_t offset = 0;
    memcpy(&cpu_->r[0], data.data() + offset, sizeof(cpu_->r)); offset += sizeof(cpu_->r);
    memcpy(&cpu_->pc, data.data() + offset, sizeof(cpu_->pc)); offset += sizeof(cpu_->pc);
    memcpy(&cpu_->sp, data.data() + offset, sizeof(cpu_->sp)); offset += sizeof(cpu_->sp);
    memcpy(&cpu_->f, data.data() + offset, sizeof(cpu_->f)); offset += sizeof(cpu_->f);
    if (cpu_->m)
        memcpy(cpu_->m, data.data() + offset, MEM_SIZE);
    cpu_rec_invalidate(cpu_, 0, MEM_SIZE);
    {
        std::lock_guard<std::mutex> slk(stop_mtx_);
        noteStopLocked("restored", cpu_->pc);
    }
    stop_cv_.notify_all();
    return true;
}

// Control hook function pointers with safe no-op defaults
namespace {
    static void (*g_resume_fn)(void) = [](){ };
    static void (*g_pause_fn)(void) = [](){ };
    static void (*g_step_fn)(void) = [](){ };
}

void inspector_set_control_hooks(void (*resume_fn)(void), void (*pause_fn)(void), void (*step_fn)(void)) {
    if (resume_fn) g_resume_fn = resume_fn;
    if (pause_fn) g_pause_fn = pause_fn;
    if (step_fn) g_step_fn = step_fn;
}

void Inspector::run() {
    g_resume_fn();
}

void Inspector::pause() {
    g_pause_fn();
    {
        std::lock_guard<std::mutex> lk(stop_mtx_);
        noteStopLocked("paused", cpu_ ? cpu_->pc : 0);
    }
    stop_cv_.notify_all();
}

void Inspector::step(size_t count) {
    for (size_t i = 0; i < std::max<size_t>(count, 1); ++i)
        g_step_fn();
    {
        std::lock_guard<std::mutex> lk(stop_mtx_);
        noteStopLocked("step", cpu_ ? cpu_->pc : 0);
    }
    stop_cv_.notify_all();
}

void Inspector::noteStopLocked(const std::string& reason, uint16_t pc) {
    stop_state_.reason = reason;
    stop_state_.pc = pc;
    ++stop_state_.sequence;
}

void Inspector::pushEvent(const Event& ev) {
    std::lock_guard<std::mutex> lk(event_mtx_);
    if (ev.type == "breakpoint" || ev.type == "watchpoint" || ev.type == "paused" || ev.type == "stopped") {
        {
            std::lock_guard<std::mutex> slk(stop_mtx_);
            noteStopLocked(ev.type, cpu_ ? cpu_->pc : 0);
        }
        stop_cv_.notify_all();
    }
    if (event_subscribers_ > 0) {
        if (event_q_.size() >= MAX_EVENT_QUEUE) event_q_.pop_front();
        event_q_.push_back(ev);
        event_cv_.notify_one();
    }

    subscriptions_.erase(std::remove_if(subscriptions_.begin(), subscriptions_.end(),
        [](const std::weak_ptr<EventSubscription>& weak) { return weak.expired(); }),
        subscriptions_.end());
    for (auto& weak : subscriptions_) {
        if (auto sub = weak.lock()) {
            std::lock_guard<std::mutex> qlk(sub->mtx);
            if (sub->closed) continue;
            if (sub->q.size() >= MAX_EVENT_QUEUE) sub->q.pop_front();
            sub->q.push_back(ev);
            sub->cv.notify_one();
        }
    }
}

bool Inspector::popEventBlocking(Event &out, int timeout_ms) {
    std::unique_lock<std::mutex> lk(event_mtx_);
    if (event_q_.empty()) {
        if (timeout_ms <= 0)
            event_cv_.wait(lk, [this]{ return !event_q_.empty(); });
        else {
            auto dur = std::chrono::milliseconds(timeout_ms);
            if (!event_cv_.wait_for(lk, dur, [this]{ return !event_q_.empty(); }))
                return false; // timeout
        }
    }
    if (event_q_.empty()) return false;
    out = event_q_.front();
    event_q_.pop_front();
    return true;
}

void Inspector::eventSubscriberAttached() {
    std::lock_guard<std::mutex> lk(event_mtx_);
    ++event_subscribers_;
}

void Inspector::eventSubscriberDetached() {
    std::lock_guard<std::mutex> lk(event_mtx_);
    if (event_subscribers_ > 0) --event_subscribers_;
}

bool Inspector::hasEventSubscribers() {
    std::lock_guard<std::mutex> lk(event_mtx_);
    return event_subscribers_ > 0 || !subscriptions_.empty();
}

std::shared_ptr<Inspector::EventSubscription> Inspector::createEventSubscription() {
    auto sub = std::make_shared<EventSubscription>();
    std::lock_guard<std::mutex> lk(event_mtx_);
    subscriptions_.push_back(sub);
    return sub;
}

void Inspector::closeEventSubscription(const std::shared_ptr<EventSubscription>& sub) {
    if (!sub) return;
    {
        std::lock_guard<std::mutex> lk(sub->mtx);
        sub->closed = true;
    }
    sub->cv.notify_all();
}

bool Inspector::popEventBlocking(const std::shared_ptr<EventSubscription>& sub, Event &out, int timeout_ms) {
    if (!sub) return false;
    std::unique_lock<std::mutex> lk(sub->mtx);
    auto pred = [&] { return sub->closed || !sub->q.empty(); };
    if (!pred()) {
        if (timeout_ms <= 0)
            sub->cv.wait(lk, pred);
        else if (!sub->cv.wait_for(lk, std::chrono::milliseconds(timeout_ms), pred))
            return false;
    }
    if (sub->closed || sub->q.empty()) return false;
    out = sub->q.front();
    sub->q.pop_front();
    return true;
}

void Inspector::wakeEventWaiters() {
    // Notify any threads waiting for events so they can wake and observe shutdown
    std::lock_guard<std::mutex> lk(event_mtx_);
    event_cv_.notify_all();
    for (auto& weak : subscriptions_) {
        if (auto sub = weak.lock()) sub->cv.notify_all();
    }
    stop_cv_.notify_all();
}

} // namespace mash16
