#include "haq.h"

#if HAQ_ENABLE_SCHEMA
void haq_print(const haq_schema &schema, int level = 0)
{
    printf("%*s%s %s", level * 2, "", schema.type.c_str(), schema.name.c_str());
    if (schema.fields.size()) {
        printf(" {\n");
        for (const auto &field : schema.fields) {
            haq_print(field, level + 1);
            printf("%*s", level * 2, "");
        }
        printf("}");
    }
    printf(";\n");
}
#endif