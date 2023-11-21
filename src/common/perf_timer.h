#pragma once
#include "common.h"

struct PerfTimer {
    std::string name{};
    double startedAt{};

    static std::vector<PerfTimer *> timerStack;

    PerfTimer(const std::string &name, PerfTimer *parent = 0) : name(name) {
        timerStack.push_back(this);
        startedAt = GetTime();
        char buf[256]{};
        int len = 0;
        for (const PerfTimer *timer : timerStack) {
            len += snprintf(buf + len, sizeof(buf) - len, "%7.3fs| ", startedAt - timer->startedAt);
        }
        printf("%.*s %s\n", len, buf, name.c_str());
    }

    ~PerfTimer(void) {
        double endedAt = GetTime();
        char buf[256]{};
        int len = 0;
        for (const PerfTimer *timer : timerStack) {
            len += snprintf(buf + len, sizeof(buf) - len, "%7.3fs| ", endedAt - timer->startedAt);
        }
        printf("%.*s End\n", len, buf);
        timerStack.pop_back();
    }
};