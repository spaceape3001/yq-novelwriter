////////////////////////////////////////////////////////////////////////////////
//
//  YOUR QUILL
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <stdio.h>
#include <vector>
#include <stack>
#include <map>
#include <set>

//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//      EXCEPTIONS 
//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

struct BadSlot {};

struct Syntax {
    const char* what;
    Syntax(const char* msg) : what(msg) {}
};

struct BadWrite {
    std::string file;
    BadWrite(const std::string& ch) : file(ch) {}
};

struct Help {};

struct Bad {
    const char* what;
    const char* arg;
    Bad(const char* _w) : what(_w), arg(nullptr) {}
    Bad(const char* _w, const char* _a) : what(_w), arg(_a) {}
};



//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//      STRUCTURES
//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


enum ChunkType {
    ctNone = 0,
    ctFile,
    ctNewLine,
    ctText,
    ctCommand,
    ctSlotChange,
    ctSlotNext,
    ctSelect,
    ctUmlat,
    ctGrave,
    ctAccent,
    ctCirc,
    ctTilde,
    ctCode,
    ctChar,
    ctBoldOn,
    ctBoldOff,
    ctItalicOn,
    ctItalicOff,
    ctUnderOn,
    ctUnderOff,
    ctStrikeOn,
    ctStrikeOff,
    ctSuperOn,
    ctSuperOff,
    ctSubOn,
    ctSubOff,
    ctAmp,
    ctGreat,
    ctLess,
    ctLDQuote,
    ctRDQuote,
    ctLSQuote,
    ctRSQuote,
    ctSQuote,
    ctDQuote,
    ctNDash,
    ctMDash,
    ctEllipse
};

const char* chunkTypeName(ChunkType);


struct Chunk {
    
    ChunkType           type;
    const char*         file;
    unsigned int        line;
    uint16_t            mask;
    
    std::string         text;           // could be the command if type is "Command"
    
    Chunk*              next;
    std::vector<Chunk*> args;
    unsigned int        number;
    
    Chunk(ChunkType t=ctNone);
    Chunk*              lastSibling() const;
    std::string         argText(size_t) const;
};

const char* keyFor(ChunkType);

enum OutputMode {
    omNone      = 0,
    omFull,
    omChapter,
    omSection
};

enum Align {
    AlignNone   = 0,
    AlignLeft,
    AlignCenter,
    AlignRight,
    AlignJustify
};

struct IgCase {
    bool operator()(const std::string&, const std::string&) const;
};

template <typename T>
struct Opt {
    T       value;
    bool    set;
    
    Opt(const T& t=T()) : value(t), set(false) {}
    Opt& operator=(const T& v) 
    {
        value   = v;
        set = true;
        return *this;
    }
    operator const T&() const { return value; }
};


struct Formatter {
public:
    
    struct Section;
    struct Chapter;
    struct Macro;
    struct Style;
    //struct Block;
    struct Reference;
    
    typedef std::map<std::string,Chunk*, IgCase>    ArgMap;
    
    OutputMode                              eOutMode;
    const char*                             sPrefix;
    const char*                             sSuffix;
    Chunk*                                  story;
    std::vector<Chunk*>                     acts, parts;
    std::vector<Chapter*>                   chapters;
    std::vector<Chunk*>                     footnotes;
    unsigned int                            maxSecId;
    std::map<std::string, Macro*, IgCase>   macros;
    //std::map<std::string, Block*, IgCase>   blocks;
    std::map<std::string, Style*, IgCase>   styles;
    //std::map<std::string, 
    std::set<std::string, IgCase>           formats;
    std::map<std::string, Chunk*, IgCase>   biblio;
    FILE*                                   out;
    unsigned int                            digChap, digSect;
    unsigned int                            curChap, curSect;
    bool                                    bTOC;
    bool                                    bHyperlink;
    bool                                    bNames;
    bool                                    bCSS;
    
    std::map<std::string,Chunk*,IgCase>     text;
    
    Chunk*                                  textChunk(const std::string&);
    
    
    static std::set<std::string,IgCase>     allFormats;

    Formatter();
    virtual ~Formatter ();
    
    //  This is what gets called
    virtual void        process(Chunk*);
    virtual void        finalize();
    
    
protected:    
    //virtual const char* suffix() const { return nullptr; }
    
    //  part of the internal processing ....
    void            enumerate(Chunk*);
    void            defMacros(Chunk*);
    //void            defBlocks(Chunk*);
    void            defStyles(Chunk*);
    void            defBiblio(Chunk*);
    void            defTitles(Chunk*);
    
    void            doMacro(Chunk*, Macro&, const ArgMap& args=ArgMap());
    //void            doBlock(Chunk*, Block&, const ArgMap& args=ArgMap());
    void            doStyle(Chunk*, Style&, const ArgMap& args=ArgMap());
    //void            doTable(Chunk*, const ArgMap& args=ArgMap());
    //void            doTableRow(Chunk*, const ArgMap& args=ArgMap());
    //void            doTableHeader(Chunk*, const ArgMap& args=ArgMap());
    
    bool            isOkayName(Chunk*, bool fAllowReserved=false );
    
    /*
        Ends the file
    */
    void            endFile();
    
    /*
        Renders the specified chunk and its later siblings
    */
    void            render(Chunk*, const ArgMap& args=ArgMap());
    void            renderSubs(Chunk*, const ArgMap& args=ArgMap());
    
    /*
        Treats the chunk as a command, executes it.
    */
    void            command(Chunk*, const ArgMap& args=ArgMap());



        //  -----------------------------------------------------------------
        //  Used to write stuff out 
        //  -----------------------------------------------------------------


    virtual void    prepForRender(){}

    
    /// Start of the current output file
    virtual void    writeDocStart(){}
    
    //virtual void    writeBlockStart(Chunk*, Block&){}
    //virtual void    writeBlockEnd(Chunk*, Block&){}
    
    virtual void    writeBreak(){}
    
    virtual void    writeStyleStart(Chunk*, Style&){}
    virtual void    writeStyleEnd(Chunk*, Style&){}
    
    virtual void    writeDocTitle(Chunk*){}
    
    virtual void    writeAct(Chunk*){}
    virtual void    writePart(Chunk*) {}
    virtual void    writeChapter(Chunk*) {}
    virtual void    writeCite(Chunk*) {}
    virtual void    writeSection(Chunk*) {}
    
    /// End of the current output file
    virtual void    writeDocEnd(){}
    
    /// A new line request (typically a paragraph)
    virtual void    writeNewline(){}
    
    /// Text... what it says
    virtual void    writeText(const std::string&){}
    
    /// Umlat of the specified character(s) requested
    virtual void    writeUmlat(const std::string&){}

    /// Grave of the specified character(s) requested
    virtual void    writeGrave(const std::string&){}

    /// Accent of the specified character(s) requested
    virtual void    writeAccent(const std::string&){}

    /// Circumflex of the specified character(s) requested
    virtual void    writeCirc(const std::string&){}

    /// Tilde of the specified character(s) requested
    virtual void    writeTilde(const std::string&){}

    /// Code point requested
    virtual void    writeCode(const std::string&){}
    
    virtual void    writeCharacter(const std::string&){}
    
    /// Turn on bold
    virtual void    writeBoldOn(){}
    
    /// Turn off bold
    virtual void    writeBoldOff(){}
    
    /// Turn on italic
    virtual void    writeItalicOn(){}
    
    /// Turn off italic
    virtual void    writeItalicOff(){}
    
    /// Turn on underline
    virtual void    writeUnderOn(){}
    
    /// Turn off underline
    virtual void    writeUnderOff(){}
    
    /// Turn on strike-through
    virtual void    writeStrikeOn(){}
    
    /// Turn off strike-through
    virtual void    writeStrikeOff(){}
    
    /// Turn on super-script
    virtual void    writeSuperOn(){}
    
    /// Turn off super-script
    virtual void    writeSuperOff(){}
    
    /// Turn on sub-script
    virtual void    writeSubOn(){}
    
    /// Turn off sub-script
    virtual void    writeSubOff(){}
    
    /// Write an ampersand for text
    virtual void    writeAmpersand(){}
    
    /// Write the greater sign (for text)
    virtual void    writeGreater(){}
    virtual void    writeLesser(){}
    virtual void    writeLDQuote(){}
    virtual void    writeRDQuote(){}
    virtual void    writeLSQuote(){}
    virtual void    writeRSQuote(){}
    virtual void    writeDQuote(){}
    virtual void    writeSQuote(){}
    virtual void    writeNDash(){}
    virtual void    writeMDash(){}
    virtual void    writeEllipse(){}
    virtual void    writeScene(Chunk*) {}
    virtual void    writeFootnote(Chunk*) {}
    
    virtual void    writeTableStart() {}
    virtual void    writeTableEnd() {}
    virtual void    writeTRowStart() {}
    virtual void    writeTRowEnd() {}
    virtual void    writeTColStart(){}
    virtual void    writeTColEnd(){}
    virtual void    writeTHdrStart(){}
    virtual void    writeTHdrEnd(){}
    
    virtual void    writeUListStart(){}
    virtual void    writeUListEnd(){}
    
    virtual void    writeItemStart(){}
    virtual void    writeItemEnd(){}

    virtual void    writeOListStart(){}
    virtual void    writeOListEnd(){}

    //virtual void    writeStartBlock(Chunk*) {}
    //virtual void    writeEndBlock(Chunk*) {}
    //virtual void    writeStartStyle(Chunk*) {}
    //virtual void    writeEndStyle(Chunk*) {}
};

struct Formatter::Section {
    Chunk*              chunk;
    std::vector<Chunk*> footnotes;
    std::string         filename;
    Section(Formatter*, Chunk* c=nullptr) : chunk(c) {}
};

struct Formatter::Chapter {
    Chunk*                  chunk;
    std::vector<Section*>   sections;
    std::vector<Chunk*>     footnotes;
    std::string             filename;
    Chapter(Formatter*, Chunk* c=nullptr) : chunk(c) {}
};

struct Formatter::Macro {
    Chunk*                      chunk;
    Chunk*                      definition;
    std::vector<std::string>    args;
    Macro(Formatter*, Chunk* c=nullptr) ;
};

enum TagStyle {
    tsNone = 0,
    tsDiv,
    tsSpan,
    tsBlock,
    tsTable,
    tsFake
};

struct Formatter::Style {
    Chunk*                  chunk;
    bool                    bBold, bItalic, bStrikeThru, bUnderline, bSuperscript, bSubscript, bSmallCaps;
    bool                    bForceNewline, bPrepad, bRule;  // only if "force newline"
    bool                    bBlock, bUnPar, bBorder;
    std::string             color, bgColor, bgImage, fontSize, fontFamily;
    std::string             padLeft, padRight, padTop, padBottom;
    Align                   align;
    //int                     indent;
    TagStyle                tag;        // for the html parser
    bool                    bCss;
    std::string             manual;
    
    Style(Formatter*, Chunk* c);
    virtual ~Style(){}
    
    void    defStyle(Formatter*, Chunk*);
};

//struct Formatter::Block : public Style {
    //std::string             bgColor, bgImage;
    //bool                    bUnPar;
    //int                     indent;
    
    //Block(Formatter*, Chunk* c=nullptr);
    //virtual ~Block(){}
    
    //void    defBlock(Formatter*, Chunk*);
//};


Formatter*  fmtHTML();
Formatter*  fmtAFF();
Formatter*  fmtFFN();
Formatter*  fmtAO3();
Formatter*  fmtText();
Formatter*  fmtDebug();
Formatter*  fmtLatex();
Formatter*  fmtMarkdown();
Formatter*  fmtRTF();

//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//      ROUTINES
//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

enum {
    AutoFilterSlots     = 0x1
};

Chunk*      chunkify (FILE*, const char*);
void        print(FILE*, const Chunk*);

const char*     space(const char*);
const char*     nospace(const char*);
void            estrip(char*, size_t);
bool            ispush(const char*);
Chunk*          selectSlot(Chunk* head, uint8_t slot);
bool            lessThan(const char* a, const char* b);
bool            equal(const char* a, const char* b);
bool            equal(const char* a, const std::string& b);
bool            equal(const std::string& a, const char* b);
bool            equal(const std::string& a, const std::string& b);
const char*     starts(const char* a, const char* b);
const char*     leading(const char* text);
unsigned int    digits(unsigned int n);

std::string     concatText(Chunk*, bool fRecurse=true);
std::string     legalFile(const char*);

size_t          count(Chunk*);
const char*     keyFor(ChunkType ct);
const char*     chunkTypeName(ChunkType ct);

//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//      GLOBALS
//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

extern bool         gDebug, gTrace;
extern Chunk*       gHead;
extern uint64_t     gOptions;


