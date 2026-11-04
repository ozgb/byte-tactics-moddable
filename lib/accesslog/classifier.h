#ifndef ACCESSLOG_CLASSIFIER_H
#define ACCESSLOG_CLASSIFIER_H

enum ClassifierEvidence { EVIDENCE_RAW, EVIDENCE_UNWRITTEN, EVIDENCE_UNREAD, EVIDENCE_ORDERED, EVIDENCE_KINDS };

const unsigned kNoSub = 0xFFFF;

const unsigned kPendingLabel = 10;

struct ClassifierEdge {
    unsigned writer, reader, item, label, evidence, count, firstTick;
};

class Classifier {
public:
    typedef void* (*AllocFn)(unsigned bytes);

    bool Init(unsigned items, unsigned edgeBits, AllocFn alloc);
    void BeginWindow(unsigned label);
    void ResolveLabel(unsigned label);
    void EndWindow(unsigned tick);
    void Read(unsigned item, unsigned entA, unsigned entB, unsigned sub, unsigned tick);
    void Write(unsigned item, unsigned entA, unsigned entB, unsigned sub, unsigned tick);
    void Ordered(unsigned stream, unsigned item, unsigned sub, unsigned tick);
    unsigned Slots() const { return slots; }
    bool Edge(unsigned slot, ClassifierEdge* out) const;
    bool Overflowed() const { return overflow; }

private:
    struct ItemState {
        unsigned stamp, writer, entA, entB;
        unsigned char read;
    };
    struct Slot {
        unsigned key0, key1, item, count, firstTick;
    };
    struct Stream {
        unsigned stamp, sub;
    };
    enum { kStreams = 4 };

    void Count(unsigned writer, unsigned reader, unsigned item, unsigned evidence, unsigned tick, unsigned amount);
    Slot* Find(unsigned writer, unsigned reader, unsigned item, unsigned label, unsigned evidence, unsigned* index);

    ItemState* state = 0;
    unsigned* written = 0;
    unsigned writtenCount = 0;
    unsigned items = 0;
    Slot* table = 0;
    unsigned slots = 0;
    unsigned used = 0;
    unsigned* pending = 0;
    unsigned pendingCount = 0;
    unsigned pendingMax = 0;
    Stream streams[kStreams] = {};
    unsigned stamp = 1;
    unsigned label = 0;
    bool overflow = false;
};

inline bool Classifier::Init(unsigned itemCount, unsigned edgeBits, AllocFn alloc)
{
    items = itemCount;
    slots = 1u << edgeBits;
    state = (ItemState*)alloc(sizeof(ItemState) * (items + 1));
    written = (unsigned*)alloc(sizeof(unsigned) * (items + 1));
    table = (Slot*)alloc(sizeof(Slot) * slots);
    pendingMax = 1u << 16;
    pending = (unsigned*)alloc(sizeof(unsigned) * pendingMax);
    return state != 0 && written != 0 && table != 0 && pending != 0;
}

inline Classifier::Slot* Classifier::Find(unsigned writer, unsigned reader, unsigned item, unsigned lab, unsigned evidence, unsigned* index)
{
    unsigned key0 = writer | reader << 16;
    unsigned key1 = lab | evidence << 8 | 0x80000000u;
    unsigned h = (key0 * 2654435761u) ^ (item * 40503u) ^ (key1 * 2246822519u);
    h ^= h >> 15;
    for (unsigned n = 0; n < slots; n++) {
        unsigned i = (h + n) & (slots - 1);
        Slot* s = &table[i];
        if (s->key1 == 0) {
            if (used >= slots - slots / 8) {
                overflow = true;
                return 0;
            }
            s->key0 = key0;
            s->key1 = key1;
            s->item = item;
            used++;
            *index = i;
            return s;
        }
        if (s->key0 == key0 && s->key1 == key1 && s->item == item) {
            *index = i;
            return s;
        }
    }
    overflow = true;
    return 0;
}

inline void Classifier::Count(unsigned writer, unsigned reader, unsigned item, unsigned evidence, unsigned tick, unsigned amount)
{
    unsigned index = 0;
    Slot* s = Find(writer, reader, item, label, evidence, &index);
    if (s == 0)
        return;
    if (s->count == 0) {
        s->firstTick = tick;
        if (label == kPendingLabel && pendingCount < pendingMax)
            pending[pendingCount++] = index;
    }
    s->count += amount;
}

inline void Classifier::BeginWindow(unsigned lab)
{
    stamp++;
    label = lab;
    writtenCount = 0;
}

inline void Classifier::ResolveLabel(unsigned lab)
{
    if (label != kPendingLabel) {
        return;
    }
    unsigned n = pendingCount;
    pendingCount = 0;
    label = lab;
    for (unsigned i = 0; i < n; i++) {
        Slot* from = &table[pending[i]];
        if (from->count == 0)
            continue;
        unsigned writer = from->key0 & 0xFFFF, reader = from->key0 >> 16;
        unsigned evidence = (from->key1 >> 8) & 0xFF;
        unsigned count = from->count, first = from->firstTick, item = from->item;
        from->count = 0;
        unsigned index = 0;
        Slot* to = Find(writer, reader, item, lab, evidence, &index);
        if (to == 0)
            continue;
        if (to->count == 0 || first < to->firstTick)
            to->firstTick = first;
        to->count += count;
    }
}

inline void Classifier::EndWindow(unsigned tick)
{
    for (unsigned i = 0; i < writtenCount; i++) {
        ItemState* s = &state[written[i]];
        if (!s->read)
            Count(s->writer, kNoSub, written[i], EVIDENCE_UNREAD, tick, 1);
    }
    writtenCount = 0;
}

inline void Classifier::Read(unsigned item, unsigned entA, unsigned entB, unsigned sub, unsigned tick)
{
    ItemState* s = &state[item];
    if (s->stamp == stamp) {
        if (s->writer != sub || s->entA != entA || s->entB != entB)
            Count(s->writer, sub, item, EVIDENCE_RAW, tick, 1);
        s->read = 1;
    } else {
        Count(kNoSub, sub, item, EVIDENCE_UNWRITTEN, tick, 1);
    }
}

inline void Classifier::Write(unsigned item, unsigned entA, unsigned entB, unsigned sub, unsigned tick)
{
    (void)tick;
    ItemState* s = &state[item];
    if (s->stamp != stamp) {
        s->stamp = stamp;
        written[writtenCount++] = item;
    }
    s->writer = sub;
    s->entA = entA;
    s->entB = entB;
    s->read = 0;
}

inline void Classifier::Ordered(unsigned stream, unsigned item, unsigned sub, unsigned tick)
{
    Stream* st = &streams[stream];
    if (st->stamp == stamp && st->sub != sub)
        Count(st->sub, sub, item, EVIDENCE_ORDERED, tick, 1);
    st->stamp = stamp;
    st->sub = sub;
}

inline bool Classifier::Edge(unsigned slot, ClassifierEdge* out) const
{
    const Slot* s = &table[slot];
    if (s->key1 == 0 || s->count == 0)
        return false;
    out->writer = s->key0 & 0xFFFF;
    out->reader = s->key0 >> 16;
    out->item = s->item;
    out->label = s->key1 & 0xFF;
    out->evidence = (s->key1 >> 8) & 0xFF;
    out->count = s->count;
    out->firstTick = s->firstTick;
    return true;
}

#endif
