#include "inspector.h"
#include <cstring>
#include <algorithm>
#include <sstream>
#include <chrono>

namespace mash16 {

Inspector::Inspector(cpu_state *cpu) : cpu_(cpu) {}

Inspector::Registers Inspector::getRegisters() {
    std::lock_guard<std::mutex> lk(mtx_);
    Registers regs;
    for (int i = 0; i < 16; ++i) regs.r[i] = cpu_->r[i];
    regs.pc = cpu_->pc;
    regs.sp = cpu_->sp;
    regs.f = cpu_->f;
    return regs;
}

std::vector<uint8_t> Inspector::readMemory(uint16_t addr, size_t size) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (!cpu_ || !cpu_->m) return {};
    if (addr >= MEM_SIZE) return {};
    size_t avail = std::min<size_t>(size, MEM_SIZE - addr);
    return std::vector<uint8_t>(cpu_->m + addr, cpu_->m + addr + avail);
}

bool Inspector::writeMemory(uint16_t addr, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (!cpu_ || !cpu_->m) return false;
    if (addr >= MEM_SIZE) return false;
    size_t avail = std::min<size_t>(data.size(), MEM_SIZE - addr);
    memcpy(cpu_->m + addr, data.data(), avail);
    // Emit memory write event
    Event ev;
    ev.type = "mem_write";
    std::ostringstream o;
    o << "{\"type\":\"mem_write\",\"addr\":" << addr << ",\"bytes\": [";
    for (size_t i = 0; i < avail; ++i) { if (i) o << ","; o << (int)data[i]; }
    o << "]}";
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

std::vector<uint8_t> Inspector::snapshot() {
    std::lock_guard<std::mutex> lk(mtx_);
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
    std::lock_guard<std::mutex> lk(mtx_);
    size_t expected = sizeof(cpu_->r) + sizeof(cpu_->pc) + sizeof(cpu_->sp) + sizeof(cpu_->f) + MEM_SIZE;
    if (data.size() < expected) return false;
    size_t offset = 0;
    memcpy(&cpu_->r[0], data.data() + offset, sizeof(cpu_->r)); offset += sizeof(cpu_->r);
    memcpy(&cpu_->pc, data.data() + offset, sizeof(cpu_->pc)); offset += sizeof(cpu_->pc);
    memcpy(&cpu_->sp, data.data() + offset, sizeof(cpu_->sp)); offset += sizeof(cpu_->sp);
    memcpy(&cpu_->f, data.data() + offset, sizeof(cpu_->f)); offset += sizeof(cpu_->f);
    if (cpu_->m)
        memcpy(cpu_->m, data.data() + offset, MEM_SIZE);
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
}

void Inspector::step() {
    g_step_fn();
}

void Inspector::pushEvent(const Event& ev) {
    std::lock_guard<std::mutex> lk(event_mtx_);
    if (event_subscribers_ == 0) return;
    if (event_q_.size() >= MAX_EVENT_QUEUE) {
        event_q_.pop_front();
    }
    event_q_.push_back(ev);
    event_cv_.notify_one();
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
    return event_subscribers_ > 0;
}

void Inspector::wakeEventWaiters() {
    // Notify any threads waiting for events so they can wake and observe shutdown
    std::lock_guard<std::mutex> lk(event_mtx_);
    event_cv_.notify_all();
}

} // namespace mash16
