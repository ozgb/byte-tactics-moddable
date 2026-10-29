#ifndef MISSION_TYPE_H
#define MISSION_TYPE_H

struct OrderTableEntry;

class MissionType {
public:
    unsigned char index;
    MissionType() : index(0) {}
    explicit MissionType(unsigned char i) : index(i) {}
    MissionType(const char* name);
    int operator==(const MissionType& other) const { return index == other.index; }
    OrderTableEntry* GetTableEntry();
};

#endif
