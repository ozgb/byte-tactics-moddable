#ifndef STRING_REF_VIEW_H
#define STRING_REF_VIEW_H

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
