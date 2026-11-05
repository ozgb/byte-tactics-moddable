#ifndef ACCESSLOG_CLASSIFIER_H
#define ACCESSLOG_CLASSIFIER_H

enum ClassifierEvidence {
    EVIDENCE_RAW,
    EVIDENCE_UNWRITTEN,
    EVIDENCE_UNREAD,
    EVIDENCE_ORDERED,
    EVIDENCE_CROSS,
    EVIDENCE_CARRIED,
    EVIDENCE_KINDS
};

enum ClassifierReadFlags { READ_EARLIER_ENTITY = 1, READ_SAME_RECORD = 2 };

const unsigned kNoSub = 0xFFFF;

const unsigned kPendingLabel = 10;

struct ClassifierEdge {
    unsigned writer, reader, item, label, from, evidence, count, firstTick;
};

class Classifier {
public:
    typedef void* (*AllocFn)(unsigned bytes);

    bool Init(unsigned items, unsigned edgeBits, AllocFn alloc);
    void BeginWindow(unsigned label);
    void ResolveLabel(unsigned label);
    void EndWindow(unsigned tick);
    unsigned Read(unsigned item, unsigned entA, unsigned entB, unsigned sub, unsigned tick);
    void Write(unsigned item, unsigned entA, unsigned entB, unsigned sub, unsigned tick);
    void Ordered(unsigned stream, unsigned item, unsigned sub, unsigned tick);
    void Entity(unsigned ordinal) { entity = ordinal; }
    unsigned Slots() const { return slots; }
    bool Edge(unsigned slot, ClassifierEdge* out) const;
    bool Overflowed() const { return overflow; }

private:
    struct ItemState {
        unsigned stamp, writer, entA, entB, writerOrd;
        unsigned tickEpoch, tickWriter, tickWin;
        unsigned char read;
    };
    struct Slot {
        unsigned key0, key1, item, count, firstTick;
    };
    struct Stream {
        unsigned stamp, sub;
    };
    enum { kStreams = 8, kWindows = 32 };

    void Count(unsigned writer, unsigned reader, unsigned item, unsigned evidence, unsigned tick, unsigned amount, unsigned from = 0);
    Slot* Find(unsigned writer, unsigned reader, unsigned item, unsigned label, unsigned from, unsigned evidence, unsigned* index);

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
    unsigned epoch = 1;
    unsigned window = 0;
    unsigned entity = 0;
    unsigned windowLabels[2][kWindows] = {};
    unsigned current = 0;
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

inline Classifier::Slot* Classifier::Find(unsigned writer, unsigned reader, unsigned item, unsigned lab, unsigned from, unsigned evidence, unsigned* index)
{
    unsigned key0 = writer | reader << 16;
    unsigned key1 = lab | evidence << 8 | from << 16 | 0x80000000u;
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

inline void Classifier::Count(unsigned writer, unsigned reader, unsigned item, unsigned evidence, unsigned tick, unsigned amount, unsigned from)
{
    unsigned index = 0;
    Slot* s = Find(writer, reader, item, label, from, evidence, &index);
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
    entity = 0;
    if (lab == kPendingLabel) {
        epoch++;
        current ^= 1;
        window = 0;
    } else if (window < kWindows - 1) {
        window++;
    }
    windowLabels[current][window] = lab;
}

inline void Classifier::ResolveLabel(unsigned lab)
{
    if (label != kPendingLabel) {
        return;
    }
    unsigned n = pendingCount;
    pendingCount = 0;
    label = lab;
    windowLabels[current][window] = lab;
    for (unsigned i = 0; i < n; i++) {
        Slot* from = &table[pending[i]];
        if (from->count == 0)
            continue;
        unsigned writer = from->key0 & 0xFFFF, reader = from->key0 >> 16;
        unsigned evidence = (from->key1 >> 8) & 0xFF;
        unsigned phaseFrom = (from->key1 >> 16) & 0xFF;
        unsigned count = from->count, first = from->firstTick, item = from->item;
        from->count = 0;
        unsigned index = 0;
        Slot* to = Find(writer, reader, item, lab, phaseFrom, evidence, &index);
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

inline unsigned Classifier::Read(unsigned item, unsigned entA, unsigned entB, unsigned sub, unsigned tick)
{
    ItemState* s = &state[item];
    unsigned flags = 0;
    if (s->stamp == stamp) {
        if (s->writer != sub || s->entA != entA || s->entB != entB)
            Count(s->writer, sub, item, EVIDENCE_RAW, tick, 1);
        s->read = 1;
        if (s->writerOrd != 0 && s->writerOrd < entity) {
            flags |= READ_EARLIER_ENTITY;
            if (s->entA == entA && s->entB == entB)
                flags |= READ_SAME_RECORD;
        }
    } else {
        Count(kNoSub, sub, item, EVIDENCE_UNWRITTEN, tick, 1);
        if (s->tickEpoch == epoch && s->tickWriter != sub) {
            Count(s->tickWriter, sub, item, EVIDENCE_CROSS, tick, 1, windowLabels[current][s->tickWin]);
        } else if (s->tickEpoch != 0 && s->tickEpoch + 1 == epoch && s->tickWriter != sub) {
            unsigned from = windowLabels[current ^ 1][s->tickWin];
            if (label == kPendingLabel || from > label)
                Count(s->tickWriter, sub, item, EVIDENCE_CARRIED, tick, 1, from);
        }
    }
    return flags;
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
    s->writerOrd = entity;
    s->tickEpoch = epoch;
    s->tickWriter = sub;
    s->tickWin = window;
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
    out->from = (s->key1 >> 16) & 0xFF;
    out->evidence = (s->key1 >> 8) & 0xFF;
    out->count = s->count;
    out->firstTick = s->firstTick;
    return true;
}

#endif
