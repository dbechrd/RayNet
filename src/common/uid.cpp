#include "uid.h"
#include "common.h"
#include <cassert>
#include <ctime>
#include <cmath>

// This is port of Firebase's Push ID algorithm
// https://gist.github.com/mikelehen/3596a30bd69384624c11

/**
* Fancy ID generator that creates 20-character string identifiers with the following properties:
*
* 1. They're based on timestamp so that they sort *after* any existing ids.
* 2. They contain 72-bits of random data after the timestamp so that IDs won't collide with other clients' IDs.
* 3. They sort *lexicographically* (so the timestamp is converted to characters that will sort properly).
* 4. They're monotonically increasing.  Even if you generate more than one in the same timestamp, the
*    latter ones will sort after the former ones.  We do this by using the previous random bits
*    but "incrementing" them by 1 (only in the case of a timestamp collision).
*/
UID NextUID(void) {
    // Modeled after base64 web-safe chars, but ordered by ASCII.
    static const char PUSH_CHARS[] = "-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

    // Timestamp of last push, used to prevent local collisions if you push twice in one ms.
    static time_t lastPushTime;
    static char lastRandChars[UID_RANDOM_BYTES];

#if 0
    // Prevents compiler from optimizing it out for debugging -_-
    static UID uid = { 0 };
    memset(uid.bytes, 0, 20);
#else
    UID uid = { 0 };
#endif

    // 64-bit timestamp gets turned into 8 characters
    time_t now = time(0) * 1000;

    bool duplicateTime = (now == lastPushTime);
    lastPushTime = now;

    assert(sizeof(time_t) <= UID_TIMESTAMP_BYTES);

    // Write timestamp from right-to-left, starting at 8th byte
    // For 32-bit timestamps, it leaves the 4 leftmost bytes as zero
    for (int i = sizeof(now) - 1; i >= 0; i--) {
        //printf("%10lld %10lld %10lld\n", now, now % 64, now / 64);
        int offset = UID_TIMESTAMP_BYTES - sizeof(now);
        uid.bytes[i + offset] = PUSH_CHARS[now % 64];
        now /= 64;
    }

    assert(now == 0);

    // We generate 72-bits of randomness which get turned into 12 characters and appended to the
    // timestamp to prevent collisions with other clients.  We store the last characters we
    // generated because in the event of a collision, we'll use those same characters except
    // "incremented" by one.
    if (!duplicateTime) {
        for (int i = 0; i < UID_RANDOM_BYTES; i++) {
            lastRandChars[i] = GetRandomValue(0, 63);
        }
    } else {
        // If the timestamp hasn't changed since last push, use the same random number, except incremented by 1.
        int i;
        for (i = UID_RANDOM_BYTES - 1; i >= 0 && lastRandChars[i] == 63; i--) {
            lastRandChars[i] = 0;
        }
        // Prevent buffer underrun on overflow (when incrementing "zzzzzzzzzzzz")
        // Warning: If this gets skipped, the id is no longer guaranteed to be unique,
        // but most likely will be for quite awhile (it depends on where the random
        // value for this millisecond started in the range).
        if (i >= 0) {
            lastRandChars[i]++;
        }
    }
    for (int i = 0; i < UID_RANDOM_BYTES; i++) {
        uid.bytes[UID_TIMESTAMP_BYTES + i] = PUSH_CHARS[lastRandChars[i]];
    }
    //printf("%.*s\n", (int)sizeof(uid.bytes), uid.bytes);

    return uid;
};