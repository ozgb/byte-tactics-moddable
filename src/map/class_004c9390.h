#ifndef CLASS_004C9390_H
#define CLASS_004C9390_H

class StringRefView {
public:
    char* data;
    StringRefView(StringRefView&);
    ~StringRefView(void) { ReleaseRef(); }
    StringRefView(void);
    StringRefView* Assign(StringRefView*);
    void ReleaseRef(void);
    StringRefView& operator=(StringRefView&);
};

#endif
