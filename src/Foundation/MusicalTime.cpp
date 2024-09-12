#include "MusicalTime.h"

musical_time_t time_in_bar(musical_time_t time)
{
    return time % 1920;
}