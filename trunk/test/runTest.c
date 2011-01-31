#include <tchar.h>
#include <conio.h>
#include "MapFile_Test.h"

int _tmain(int argc, _TCHAR **argv)
{
    lcut_ts_t *suite = NULL;

    LCUT_TEST_BEGIN("GetImage", NULL, NULL);
    
    LCUT_TS_ADD(CreateMapFileTestSuite());

    LCUT_TEST_RUN();
    LCUT_TEST_REPORT();
    LCUT_TEST_END();

    _getch();

    LCUT_TEST_RESULT();

    return 0;
}