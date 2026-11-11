#ifndef INTERFACES_STATE_WRITER_H
#define INTERFACES_STATE_WRITER_H

// Major version of the state writer interface, changed when a slot is removed or its signature changes.
#define STATE_WRITER_VERSION_MAJOR 1
// Minor version of the state writer interface, changed when a slot is added.
#define STATE_WRITER_VERSION_MINOR 0

// What a handle written to a StateWriter stands for; the host turns it into the same stable ID in every build.
enum StateHandleKind {
    // An object the host does not name: only whether it is null is recorded.
    STATE_HANDLE_HEAP = 1,
    // A unit slot, as the engine hands it out.
    STATE_HANDLE_UNIT = 2,
    // A movement class, as the engine hands it out.
    STATE_HANDLE_MOVE_CLASS = 11,
    // An object of the module's own, named by its index in the module's own table; STATE_NO_INDEX for none.
    STATE_HANDLE_OWN = 10
};

// The value of an own handle that points at nothing.
#define STATE_NO_INDEX 0xffffffffu

// The functions the host gives a module to write its private state as records; a field name is a fixed string with at most one %u, filled with the index, and a null name hashes the value without dumping it.
struct StateWriter {
    // Starts a record of a section of the trace, named name in the dump; ends the record that was open.
    void (__stdcall* Begin)(int section, const char* name);
    // Writes an integer of size 1, 2 or 4 bytes; it is hashed on its own, in the byte order of the machine.
    void (__stdcall* Int)(const char* field, int index, int size, unsigned int value);
    // Writes size bytes; they are hashed on their own, in eight-byte words from the first byte and then singly.
    void (__stdcall* Bytes)(const char* field, int index, const void* data, int size);
    // Writes a handle of a StateHandleKind; value is the handle as the engine gave it, 0 for none.
    void (__stdcall* Handle)(const char* field, int index, int kind, unsigned int value);
    // Makes the next count Int and Bytes writes one run, hashed as if they were one Bytes of their bytes; no Handle or Begin may come between them.
    void (__stdcall* Run)(int count);
};

#endif
