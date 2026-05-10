#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <vector>
#include <mutex>
#include <cstdint>
#include <condition_variable>
#include <deque>
#include <string>
#include "cpu.h"

namespace mash16 {

class Inspector {
public:
    struct Event {
        std::string type;
        std::string payload; // JSON fragment or full JSON event
    };

    struct Registers {
        int16_t r[16];
        uint16_t pc;
        uint16_t sp;
        flags f;
    };

    explicit Inspector(cpu_state *cpu);

    Registers getRegisters();
    std::vector<uint8_t> readMemory(uint16_t addr, size_t size);
    bool writeMemory(uint16_t addr, const std::vector<uint8_t>& data);

    void setBreakpoint(uint16_t addr);
    void clearBreakpoint(uint16_t addr);
    std::vector<uint16_t> listBreakpoints();

    std::vector<uint8_t> snapshot();
    bool restore(const std::vector<uint8_t>&);

    /* Execution control helpers that interact with the main emulation loop. */
    void run();
    void pause();
    void step();

    /* Event queue API for streaming execution events. */
    void pushEvent(const Event& ev);
    bool popEventBlocking(Event &out, int timeout_ms);
    void eventSubscriberAttached();
    void eventSubscriberDetached();
    bool hasEventSubscribers();

    /* Wake any threads waiting on the event queue (used during shutdown). */
    void wakeEventWaiters();

    static constexpr size_t MAX_EVENT_QUEUE = 4096;

private:
    cpu_state *cpu_;
    std::mutex mtx_;
    std::vector<uint16_t> breakpoints_;

    /* Event queue */
    std::mutex event_mtx_;
    std::condition_variable event_cv_;
    std::deque<Event> event_q_;
    size_t event_subscribers_ = 0;
};

    /* Control hooks implemented in main.cpp to allow the Inspector to control
     * the emulation loop. These are declared here for simplicity and defined
     * in main when BUILD_INSPECTOR is enabled. */
    /* Control hook setter: allows main to provide implementations for resume/pause/step
     * without introducing link-time dependencies into the core static library. */
    void inspector_set_control_hooks(void (*resume_fn)(void), void (*pause_fn)(void), void (*step_fn)(void));

} // namespace mash16

#endif // INSPECTOR_H
