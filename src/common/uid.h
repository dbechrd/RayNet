#pragma once

#define UID_TIMESTAMP_BYTES 8
#define UID_RANDOM_BYTES 4  // was 12 originally

struct UID {
    char bytes[UID_TIMESTAMP_BYTES + UID_RANDOM_BYTES];
};

UID NextUID(void);