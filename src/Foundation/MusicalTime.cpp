#include "MusicalTime.h"

musical_time_t time_in_bar(musical_time_t time)
{
    musical_time_t result = time % 1920;
    // C++のモジュロは負の値を返す場合があるので、正の値にラップする
    if (result < 0) result += 1920;
    return result;
}