#pragma once
#include "common.h"

struct PerfTimer {
    std::string name{};
    double startedAt{};

    static std::vector<PerfTimer *> timerStack;

    PerfTimer(const std::string &name) : name(name) {
        timerStack.push_back(this);
        startedAt = GetTime();
        char buf[256]{};
        int len = 0;
        for (const PerfTimer *timer : timerStack) {
            const double s = startedAt - timer->startedAt;
            const double ms = fmod(s, 1) * 1000;
            const double us = fmod(ms, 1) * 1000;
            len += snprintf(buf + len, sizeof(buf) - len, "%03d.%03d,%03d| ", (int)s, (int)ms, (int)us);
        }
        printf("%.*s %s\n", len, buf, name.c_str());
    }

    ~PerfTimer(void) {
        double endedAt = GetTime();
        char buf[256]{};
        int len = 0;
        for (const PerfTimer *timer : timerStack) {
            const double s = endedAt - timer->startedAt;
            const double ms = fmod(s, 1) * 1000;
            const double us = fmod(ms, 1) * 1000;
            len += snprintf(buf + len, sizeof(buf) - len, "%03d.%03d,%03d| ", (int)s, (int)ms, (int)us);
        }
        printf("%.*s End\n", len, buf);
        timerStack.pop_back();
    }
};