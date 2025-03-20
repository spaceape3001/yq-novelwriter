////////////////////////////////////////////////////////////////////////////////
//
//  YOUR QUILL
//
////////////////////////////////////////////////////////////////////////////////

#include "common.hpp"

struct Text : public Formatter {
    Text()
    {
        sSuffix     = ".txt";
        formats.insert("text");
    }
    
    virtual ~Text()
    {
    }

};

Formatter*  fmtText()
{
    return new Text;
}

