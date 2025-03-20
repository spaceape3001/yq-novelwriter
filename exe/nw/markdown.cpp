////////////////////////////////////////////////////////////////////////////////
//
//  YOUR QUILL
//
////////////////////////////////////////////////////////////////////////////////

#include "common.hpp"

struct Markdown : public Formatter {
    Markdown()
    {
        sSuffix = ".md";
        formats.insert("markdown");
    }
    
    virtual ~Markdown()
    {
    }
};

Formatter*  fmtMarkdown()
{
    return new Markdown;
}

