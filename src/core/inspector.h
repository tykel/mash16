#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <vector>
#include <mutex>
#include <cstdint>
#include <condition_variable>
#include <deque>
#include <string>
#include <map>
#include <memory>
#include <optional>
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

    struct Instruction {
        uint16_t addr = 0;
        uint32_t raw = 0;
        uint8_t op = 0;
        instr_type type = OP_NONE;
        std::string mnemonic;
        std::string text;
        std::optional<uint16_t> immediate;
        std::string symbol;
    };

    struct StopState {
        std::string reason = "unknown";
        uint16_t pc = 0;
        uint64_t sequence = 0;
    };

    struct EventSubscription;

    explicit Inspector(cpu_state *cpu, std::recursive_mutex *cpu_mtx = nullptr);

    Registers getRegisters();
    bool writeRegister(const std::string& name, int32_t value);
    std::vector<uint8_t> readMemory(uint16_t addr, size_t size);
    bool writeMemory(uint16_t addr, const std::vector<uint8_t>& data);
    bool setControllerButton(uint32_t player, const std::string& button, bool pressed);

    void setBreakpoint(uint16_t addr);
    void clearBreakpoint(uint16_t addr);
    std::vector<uint16_t> listBreakpoints();

    void setWatchpoint(uint16_t addr);
    void clearWatchpoint(uint16_t addr);
    std::vector<uint16_t> listWatchpoints();

    void setSymbol(uint16_t addr, const std::string& name);
    void clearSymbols();
    std::map<uint16_t, std::string> listSymbols();
    std::optional<uint16_t> resolveSymbol(const std::string& name);
    std::optional<uint16_t> resolveAddressOrSymbol(const std::string& text);

    Instruction disassembleOne(uint16_t addr);
    std::vector<Instruction> disassemble(uint16_t addr, size_t count);

    StopState stopState();
    bool waitStopped(StopState& out, int timeout_ms, uint64_t after_sequence = 0);

    std::vector<uint8_t> snapshot();
    bool restore(const std::vector<uint8_t>&);

    /* Execution control helpers that interact with the main emulation loop. */
    void run();
    void pause();
    void step(size_t count = 1);

    /* Event queue API for streaming execution events. */
    void pushEvent(const Event& ev);
    bool popEventBlocking(Event &out, int timeout_ms);
    void eventSubscriberAttached();
    void eventSubscriberDetached();
    bool hasEventSubscribers();
    std::shared_ptr<EventSubscription> createEventSubscription();
    void closeEventSubscription(const std::shared_ptr<EventSubscription>& sub);
    bool popEventBlocking(const std::shared_ptr<EventSubscription>& sub, Event &out, int timeout_ms);

    /* Wake any threads waiting on the event queue (used during shutdown). */
    void wakeEventWaiters();

    static constexpr size_t MAX_EVENT_QUEUE = 4096;

private:
    cpu_state *cpu_;
    std::recursive_mutex local_cpu_mtx_;
    std::recursive_mutex *cpu_mtx_;
    std::mutex mtx_;
    std::vector<uint16_t> breakpoints_;
    std::vector<uint16_t> watchpoints_;
    std::map<uint16_t, std::string> symbols_;

    void noteStopLocked(const std::string& reason, uint16_t pc);

    /* Event queue */
    std::mutex event_mtx_;
    std::condition_variable event_cv_;
    std::deque<Event> event_q_;
    size_t event_subscribers_ = 0;
    std::vector<std::weak_ptr<EventSubscription>> subscriptions_;
    std::mutex stop_mtx_;
    std::condition_variable stop_cv_;
    StopState stop_state_;
};

    /* Control hooks implemented in main.cpp to allow the Inspector to control
     * the emulation loop. These are declared here for simplicity and defined
     * in main when BUILD_INSPECTOR is enabled. */
    /* Control hook setter: allows main to provide implementations for resume/pause/step
     * without introducing link-time dependencies into the core static library. */
    void inspector_set_control_hooks(void (*resume_fn)(void), void (*pause_fn)(void), void (*step_fn)(void));

} // namespace mash16

#endif // INSPECTOR_H
