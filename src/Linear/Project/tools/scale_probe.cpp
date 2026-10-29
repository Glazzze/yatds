#include "core/game.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

using Clock = std::chrono::steady_clock;
double elapsed(Clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

int main()
{
    std::cout << std::fixed << std::setprecision(4) << "[";
    bool first = true;
    for (int n : {100, 500, 1000}) {
        std::string track;
        for (int k = 0; k < n; ++k) track += "RGBY"[k % 4];
        const std::string queue(2000, 'B');
        zuma::Game hintGame("RGBY", track, queue, 0);
        const auto hintStart = Clock::now();
        for (int k = 0; k < 10; ++k) {
            if (hintGame.hint() != -1) throw std::runtime_error("hint mismatch");
        }
        const double hintMs = elapsed(hintStart) / 10;

        const auto reverseStart = Clock::now();
        hintGame.reverse(n / 4, n * 3 / 4);
        const double reverseMs = elapsed(reverseStart);
        if (!hintGame.undo() || hintGame.trackString() != track)
            throw std::runtime_error("reverse undo mismatch");

        const int rounds = (n + 2) / 4;
        std::string nested = "BB";
        char wrap = 'R';
        for (int r = 2; r <= rounds; ++r) {
            nested = std::string(2, wrap) + nested + std::string(2, wrap);
            wrap = wrap == 'R' ? 'B' : 'R';
        }
        const std::string prefix = std::string("GY").substr(0, n - nested.size());
        nested = prefix + nested;
        zuma::Game chain("RGBY", nested, queue, 0);
        const auto shootStart = Clock::now();
        const auto events = chain.shoot(static_cast<int>(prefix.size()) + 2 * rounds - 1);
        const double shootMs = elapsed(shootStart);
        if (events.size() != static_cast<std::size_t>(rounds))
            throw std::runtime_error("chain rounds mismatch");
        std::size_t snapshotChars = 0, removedChars = 0;
        for (const auto &e : events) {
            snapshotChars += e.trackAfter.size();
            removedChars += e.balls.size();
        }
        const auto undoStart = Clock::now();
        if (!chain.undo()) throw std::runtime_error("undo missing");
        const double undoMs = elapsed(undoStart);
        if (chain.trackString() != nested || chain.queueString() != queue
            || chain.score() != 0 || chain.moves() != 0)
            throw std::runtime_error("chain undo mismatch");
        if (!first) std::cout << ",";
        first = false;
        std::cout << "{\"n\":" << n << ",\"queue\":2000,\"rounds\":" << rounds
            << ",\"hintMeanMs\":" << hintMs << ",\"reverseMs\":" << reverseMs
            << ",\"shootMs\":" << shootMs << ",\"undoMs\":" << undoMs
            << ",\"eventSnapshotChars\":" << snapshotChars
            << ",\"removedChars\":" << removedChars << "}";
    }
    std::cout << "]\n";
}
