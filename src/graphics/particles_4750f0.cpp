struct UnknownStruct;
extern UnknownStruct* g_game;

class TimedSubParticles {
public:
    char unknown_0[8];
    unsigned int emitTime;
    bool IsEmitDue();
};

// FUNCTION: 0x4750f0
bool TimedSubParticles::IsEmitDue()
{
    return this->emitTime <= *(unsigned int*)((char*)g_game + 0x38a47);
}
