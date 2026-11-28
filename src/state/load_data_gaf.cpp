#include "load_data.h"
#include "../graphics/gaf_frame.h"
#include "../map/frame_table.h"
#include "../graphics/handle.h"

struct GafEntry;
void __stdcall InitGafSequence(Handle* ref, GafEntry* src, int index);
int __stdcall StepGafSequence(Handle* ref);
GafFrame* __stdcall GetGafSequenceFrame(Handle* ref);
GafFrame* __stdcall GetGafFrame(FrameTable* table, int index);

unsigned __stdcall LoadGetGafFrame(unsigned table, int index)
{
    return LoadAddress(GetGafFrame((FrameTable*)LoadNative(table), index));
}

void* __stdcall LoadGetGafFrameNative(unsigned table, int index)
{
    return GetGafFrame((FrameTable*)LoadNative(table), index);
}

void* __stdcall LoadGetGafSequenceFrame(void* pointer)
{
    Handle* cursor = (Handle*)pointer;
    GafEntry* linear = cursor->src;
    cursor->src = (GafEntry*)LoadNative((unsigned)linear);
    void* frame = GetGafSequenceFrame(cursor);
    cursor->src = linear;
    return frame;
}

void __stdcall LoadInitGafSequence(void* pointer, unsigned animation, int index)
{
    Handle* cursor = (Handle*)pointer;
    InitGafSequence(cursor, (GafEntry*)LoadNative(animation), index);
    if (cursor->src != nullptr)
        cursor->src = (GafEntry*)animation;
}

int __stdcall LoadStepGafSequence(void* pointer)
{
    Handle* cursor = (Handle*)pointer;
    GafEntry* linear = cursor->src;
    cursor->src = (GafEntry*)LoadNative((unsigned)linear);
    int result = StepGafSequence(cursor);
    if (cursor->src != nullptr)
        cursor->src = linear;
    return result;
}
