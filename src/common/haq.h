#pragma once

#define HAQ_COMMA ,

#define HAQ_IGNORE(...)

#define HAQ_C_TYPE(c_type, c_type_name, c_body, userdata) \
    c_type c_type_name c_body;

#define HAQ_C_FIELD(c_type, c_name, c_init, flags, userdata) \
    c_type c_name c_init;

#define HAQ_C_OTHER(x) x

#define HAQ_C(hqt, userdata) \
    hqt(HAQ_C_TYPE, HAQ_C_FIELD, HAQ_C_OTHER, userdata)

#define HAQ_ENABLE_SCHEMA 0
#if HAQ_ENABLE_SCHEMA
#include <string>
#include <vector>

// TODO(cleanup): Do I even need this? If so, probably offset and size should be added back?
struct haq_schema {
    std::string type;
    std::string name;
    int flags;
    void *userdata;
    std::vector<haq_schema> fields;
};

#define HAQ_C_SCHEMA_TYPE(c_type, c_type_name, c_fields) \
    haq_schema c_type_name##_schema { #c_type, #c_type_name, c_fields };

#define HAQ_C_SCHEMA_FIELD(c_type, c_name, c_init, flags, userdata) \
    { #c_type, #c_name, {}, flags, userdata },

#define HAQ_C_SCHEMA(hqt) \
    hqt(HAQ_C_SCHEMA_TYPE, HAQ_C_SCHEMA_FIELD)

void haq_print(const haq_schema &schema, int level = 0);
#endif