/*
 * Copyright (c) 2005-2010 Tony Bai <bigwhite.cn@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _CRT_SECURE_NO_WARNINGS

#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "..\..\ColorPrint.h"
#include "lcut.h"

static lcut_symbol_head_t _symbol_list;

static lcut_symbol_t* lookup_symbol(const char *symbol_name);
static void add_value(lcut_symbol_t *s, int obj_type, void *value);
static lcut_value_t* get_value(lcut_symbol_t *s, int obj_type);

#define RETURN_WHEN_FAILED(tc) do { \
    if ((tc)->status == TEST_CASE_FAILURE) { \
        return; \
    } \
} while(0)

#define FILL_IN_FAILED_REASON(tc, fname, fcname, lineno, reason_fmt, ...) do { \
    sprintf((tc)->fname, "%s", (fname)); \
    sprintf((tc)->fcname, "%s", (fcname)); \
    (tc)->line = (lineno); \
    sprintf((tc)->reason, (reason_fmt), __VA_ARGS__); \
    (tc)->status = TEST_CASE_FAILURE; \
} while (0)
                                  
void lcut_int_equal(lcut_tc_t *tc,
                    const int expected,
                    const int actual,
                    int lineno,
                    const char *fcname,
                    const char *fname) {
    RETURN_WHEN_FAILED(tc);

    if (expected != actual) {
        FILL_IN_FAILED_REASON(tc, fname, fcname, lineno, 
                              "期望值<%d> : 实际值<%d>",
                              expected,
                              actual);
    }
}

void lcut_int_nequal(lcut_tc_t *tc, const int expected, const int actual, int lineno,
                     const char *fcname, const char *fname) {
    RETURN_WHEN_FAILED(tc);

    if (expected == actual) {
        FILL_IN_FAILED_REASON(tc, fname, fcname, lineno, 
                              "期望值不为<%d> : 实际值<%d>",
                              expected,
                              actual);
    }
}

void lcut_str_equal(lcut_tc_t *tc, const char *expected, const char *actual, int lineno,
                    const char *fcname, const char *fname) {
    RETURN_WHEN_FAILED(tc);

    /* 
     * both pointers are NULL or pointing to the same memory 
     */ 
    if (expected == actual) return;

    if (expected && actual) {
        if (!strcmp(expected, actual)) {
            return;
        }
    }

    FILL_IN_FAILED_REASON(tc, fname, fcname, lineno,
                          "期望值<%s> : 实际值<%s>",
                           expected ? expected : "NULL", 
                           actual ? actual : "NULL");
}

void lcut_str_nequal(lcut_tc_t *tc, const char *expected, const char *actual, int lineno,
                     const char *fcname, const char *fname) {
    RETURN_WHEN_FAILED(tc);

    if (expected && actual) {
        if (strcmp(expected, actual)) {
            return;
        }
    } else if (expected != actual) {
        return;
    }

    FILL_IN_FAILED_REASON(tc, fname, fcname, lineno,
                          "期望值不为<%s> : 实际值<%s>",
                           expected ? expected : "NULL", 
                           actual ? actual : "NULL");
}

void lcut_assert(lcut_tc_t *tc, const char *msg, int condition,
                 int lineno, const char *fcname, const char *fname) {
    RETURN_WHEN_FAILED(tc);

    if (condition) {
        return;
    }

    FILL_IN_FAILED_REASON(tc, fname, fcname, lineno,
                          "%s",
                          msg);
}

void lcut_true(lcut_tc_t *tc, int condition,
               int lineno, const char *fcname, const char *fname) {
    RETURN_WHEN_FAILED(tc);

    if (condition) {
        return;
    }

    FILL_IN_FAILED_REASON(tc, fname, fcname, lineno,
                          "%s",
                          "");
}

int lcut_test_init(lcut_test_t **test, const char *title, fixture_func setup, fixture_func teardown) {
    int		        rv	= 0;
    lcut_test_t		*p	= NULL;

    p = malloc(sizeof(*p));
    if (p == NULL) {
        rv = errno;
        printf("\t[LCUT]: malloc 失败!, 错误码[%d]\n", errno);
        return rv;
    }

    memset(p, 0, sizeof(lcut_test_t));
    APR_RING_INIT(&(p->ts_head), lcut_ts_t, link);
    strncpy(p->desc, title, LCUT_MAX_NAME_LEN);
    p->setup = setup;
    p->teardown = teardown;

    APR_RING_INIT(&_symbol_list, lcut_symbol_t, link);

    (*test) = p;

    return rv;
}

void lcut_test_destroy(lcut_test_t **test) {
    lcut_test_t     *p      = (*test);
    lcut_ts_t       *ts     = NULL;
    lcut_tc_t       *tc     = NULL;
    lcut_symbol_t   *s      = NULL;
    lcut_value_t    *v      = NULL;

    while (!APR_RING_EMPTY(&_symbol_list, lcut_symbol_t, link)) {
        s = APR_RING_FIRST(&_symbol_list);
        if (s != NULL) {
            while (!APR_RING_EMPTY(&(s->value_list), lcut_value_t, link)) {
                v = APR_RING_FIRST(&(s->value_list));
                if (v != NULL) {
                    APR_RING_REMOVE(v, link);
                    free(v);
                    v = NULL;
                }
            }

            APR_RING_REMOVE(s, link);
            free(s);
            s = NULL;
        }
    }

    while (!APR_RING_EMPTY(&(p->ts_head), lcut_ts_t, link)) {
        ts = APR_RING_FIRST(&(p->ts_head));
        if (ts != NULL) {
            while (!APR_RING_EMPTY(&(ts->tc_head), lcut_tc_t, link)) {
                tc = APR_RING_FIRST(&(ts->tc_head));
                if (tc != NULL) {
                    APR_RING_REMOVE(tc, link);
                    free(tc);
                    tc = NULL;
                }
            }
        }
        APR_RING_REMOVE(ts, link);
        free(ts);
        ts = NULL;
    }

    free(p);
    (*test) = NULL;
}

int lcut_ts_init(lcut_ts_t **ts, const char *title, fixture_func setup, fixture_func teardown) {
    int         rv = 0;
    lcut_ts_t	*p = NULL;

    p = malloc(sizeof(lcut_ts_t));
    if (p == NULL) {
        rv = errno;
        printf("\t[LCUT]: malloc 失败!, 错误码[%d] n", errno);
        return rv;
    }
    memset(p, 0, sizeof(lcut_ts_t));
    APR_RING_INIT(&(p->tc_head), lcut_tc_t, link);
    strncpy(p->desc, title, LCUT_MAX_NAME_LEN);
    p->ran      = 0;
    p->failed   = 0;
    p->setup    = setup;
    p->teardown = teardown;

    (*ts) = p;

    return rv;
}

void lcut_ts_add(lcut_test_t *test, lcut_ts_t *ts) {
    lcut_ts_t	*p = ts;

    APR_RING_ELEM_INIT(p, link);
    APR_RING_INSERT_TAIL(&(test->ts_head), p, lcut_ts_t, link);
    test->suites++;
    test->cases += p->ran;
}

int lcut_tc_add(lcut_ts_t *ts,
               const char *title,
               tc_func func,
               void *para,
               fixture_func before,
               fixture_func after) {
    int	rv	= 0;
    lcut_tc_t	*tc 	= NULL;

    tc = malloc(sizeof(lcut_tc_t));
    if (tc == NULL) {
        rv = errno;
        printf("\t[LCUT]: malloc 失败!, 错误码[%d] n", rv);
        return rv;
    }
    memset(tc, 0, sizeof(lcut_tc_t));

    strncpy(tc->desc, title, LCUT_MAX_NAME_LEN);
    tc->func = func;
    tc->para = para;
    tc->before = before;
    tc->after = after;
    APR_RING_ELEM_INIT(tc, link);
    APR_RING_INSERT_TAIL(&(ts->tc_head), tc, lcut_tc_t, link);
    ts->ran++;

    return rv;
}

void lcut_test_run(lcut_test_t *test, int *result) {
    lcut_ts_t	*ts	= NULL;
    lcut_tc_t	*tc	= NULL;

    printf("%s \n", LCUT_LOGO);
    printf("单元测试 “%s”:\n\n", test->desc);

    if (test->setup != NULL) {
        test->setup();
    }

    APR_RING_FOREACH(ts, &(test->ts_head), lcut_ts_t, link) {
        if (ts != NULL) {
            printf("测试套件 <%s>: \n", ts->desc);
            if (ts->setup != NULL) {
                ts->setup();
            }
            APR_RING_FOREACH(tc, &(ts->tc_head), lcut_tc_t, link) {
                if (tc != NULL) {
                    if (tc->before != NULL) {
                        tc->before();
                    }

                    tc->func(tc, tc->para);

                    if (tc->after != NULL) {
                        tc->after();
                    }

                    if (tc->status == TEST_CASE_SUCCESS) {
                        printf(SUCCESS_TIP_FMT, tc->desc);
                        ColorPrintfA(LIME, "通过\n");
                    } else if (tc->status == TEST_CASE_FAILURE) {
                        ts->failed ++;
                        printf(FAILURE_TIP_FMT, tc->desc);
                        ColorPrintfA(RED, "失败\n");
                        ColorPrintfA(WHITE, "\n详细信息：\n");
                        ColorPrintfA(WHITE, "位置 -> ");
                        ColorPrintfA(AQUA, "%s : %d -> %s\n", tc->fname, tc->line, tc->fcname);
                        ColorPrintfA(WHITE, "原因 -> ");
                        ColorPrintfA(AQUA, "%s\n", tc->reason);
                        (*result) = TEST_CASE_FAILURE;
                    }
                }
            }
            if (ts->teardown != NULL) {
                ts->teardown();
            }
        }
    }

    if (test->teardown != NULL) {
        test->teardown();
    }

}

void lcut_test_report(lcut_test_t *test) {
    int failed_suites = 0;
    int failed_cases  = 0;
    lcut_ts_t *ts  = NULL;

    APR_RING_FOREACH(ts, &(test->ts_head), lcut_ts_t, link) {
        if (ts != NULL) {
            if (ts->failed > 0) {
                failed_suites++;
            }
            failed_cases += ts->failed;
        }
    }
    printf("\n汇总: \n");
    printf("\t测试套件数: %d \n", test->suites);
    printf("\t失败套件数: %d \n", failed_suites);
    printf("\t测试用例数: %d \n", test->cases);
    printf("\t失败用例数: %d \n", failed_cases);

    if (failed_suites == 0) {
        ColorPrintfA(LIME, GREENBAR);
    } else {
        ColorPrintfA(RED, REDBAR);
    }
}

void* lcut_mock_obj(const char *fcname,
                    int lineno,
                    const char *fname,
                    int obj_type) {
    lcut_symbol_t   *s  = NULL;
    lcut_value_t    *v  = NULL;
    void            *p;

    s = lookup_symbol(fcname);
    if (!s) {
        printf("\t[LCUT]: can't find the symbol: <%s> which is being mocked!, %d line in file %s\n", 
                fcname, lineno, fname);
        exit(EXIT_FAILURE);
    }

    v = get_value(s, obj_type);
    if (!v) {
        printf("\t[LCUT]: you have not set the value of mock obj <%s>!, %d line in file %s\n", 
                fcname, lineno, fname);
        exit(EXIT_FAILURE);
    }

    p = v->value;

    APR_RING_REMOVE(v, link);
    free(v);

    return p;
}

void lcut_mock_obj_return(const char *symbol_name,
                          void *value,
                          const char *fcname, 
                          int lineno,
                          const char *fname,
                          int obj_type) {
  
    lcut_symbol_t   *s  = NULL;

    s = lookup_symbol(symbol_name);
    if (!s) {
        errno = 0;
        s = (lcut_symbol_t*)malloc(sizeof(*s));
        if (!s) {
            printf("\t[LCUT]: malloc error!, errcode[%d]\n", errno);
            exit(EXIT_FAILURE);
        }
        memset(s, 0, sizeof(*s));
        strcpy(s->desc, symbol_name);
        APR_RING_INIT(&(s->value_list), lcut_value_t, link);
        APR_RING_INSERT_TAIL(&_symbol_list, s, lcut_symbol_t, link);
    }

    add_value(s, obj_type, value);
}

static lcut_symbol_t* lookup_symbol(const char *symbol_name) {
    lcut_symbol_t   *s  = NULL;

    APR_RING_FOREACH(s, &_symbol_list, lcut_symbol_t, link) {
        if (s != NULL) {
            if (!strcmp(s->desc, symbol_name)) {
                return s;
            }
        }
    }

    return NULL;
}

static void add_value(lcut_symbol_t *s, int obj_type, void *value) {
    lcut_value_t    *v  = NULL;

    errno = 0;    
    v = (lcut_value_t*)malloc(sizeof(*v));
    if (!v) {
        printf("\t[LCUT]: malloc error!, errcode[%d]\n", errno);
        exit(EXIT_FAILURE);
    }
    memset(v, 0, sizeof(*v));
    v->obj_type = obj_type;
    v->value    = value;

    APR_RING_INSERT_TAIL(&(s->value_list), v, lcut_value_t, link);
}

static lcut_value_t* get_value(lcut_symbol_t *s, int obj_type) {
    lcut_value_t    *v  = NULL;

    APR_RING_FOREACH(v, &(s->value_list), lcut_value_t, link) {
        if (v != NULL) {
            if (v->obj_type == obj_type) {
                return v;
            }
        }
    }

    return NULL;
}
