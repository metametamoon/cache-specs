//
// Created by metametamoon on 9/26/24.
//

#ifndef PAGE_TRAVERSER_H
#define PAGE_TRAVERSER_H

using u8 = uint8_t;
using i64 = int64_t;

double traverse_pages(i64 step, i64 repNum);
double traverse_large_memory(i64 step, i64 repNum);
double check_tag_index(i64 tag_try);
double check_assoc(i64 assoc_try);

#endif //PAGE_TRAVERSER_H
