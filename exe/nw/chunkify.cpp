////////////////////////////////////////////////////////////////////////////////
//
//  YOUR QUILL
//
////////////////////////////////////////////////////////////////////////////////

#include "common.hpp"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

bool gTrace = false;

/*
    Notes: Aug 28 2018 (am):
    
        We're going to modify this to parse w/in the line, applying the slot
        selector here.  Also, going to push the @1/@2 commands too, so
        
        @1{ text }  @2{ text }  will be legal
        
        @!  { text w/o command parsing }
        @!! { text w/o command OR slot parsing }
        
        In text an implicit *single* space after cmd will be zapped, 
        
        
        @cmd@ for spaces....
        @cmd  for no-space
        @footnote{  
        
        }  for sub...
        
        
        
        Going to modify the syntax.... keeping it simple
        
        
        @[slot]         On a line, by itself, alters the 
        
        Slot Specifier
            
        1)  @#[+-][,#[+-]] 
            
        
        
         

*/




const char* space(const char* l)
{
    while(*l && !isspace(*l))
        ++l;
    return l;
}

const char* nospace(const char* l)
{
    while(*l && isspace(*l))
        ++l;
    return l;
}

const char* eident(const char* l)
{
    while(*l && (isalnum(*l) || (*l == '_') || (*l == '-')))
        ++l;
    return l;
}

void    estrip(char* buf, size_t size)
{
    size_t  n;
    for(n=0;(n<size) && buf[n];++n)
        ;
    
    if(!n)
        return ;
    
    while(n>0){
        --n;
        if(!isspace(buf[n]))
            break;
        buf[n]  = '\0';
    }    
}


bool    ispush(const char* buf)
{
    const char* z;
    for(z = buf; *z; ++z)
        ;
    if(z == buf)
        return false;
    return (z[-1] == '{') && ((z-buf == 1) || (z[-2] != '@'));
}

bool    lessThan(const char* a, const char* b)
{
    for(;*a && *b; ++a, ++b){
        int la  = tolower(*a);
        int lb  = tolower(*b);
        if(la < lb)
            return true;
        if(lb < la)
            return false;
    }
    
    return *b ? true : false;
}


bool    equal(const char* a, const char* b)
{
    for(;*a && *b; ++a, ++b){
        if(tolower(*a) != tolower(*b))
            return false;
    }
    return (!*a) && (!*b);
}

bool        equal(const char* a, const std::string& b)
{
    return equal(a, b.c_str());
}

bool        equal(const std::string& a, const char* b)
{
    return equal(a.c_str(), b);
}

bool        equal(const std::string& a, const std::string& b)
{
    return equal(a.c_str(), b.c_str());
}

unsigned int     digits(unsigned int n)
{
    if(n < 10)
        return 1;
    if(n < 100)
        return 2;
    if(n < 1000)
        return 3;
    if(n < 10000)
        return 4;
    if(n < 100000)
        return 5;
    if(n < 1000000)
        return 6;
    if(n < 10000000)
        return 7;
    if(n < 100000000)
        return 8;
    return 9;
}


//  returns end of A or B if matches
const char* starts(const char* a, const char* b)
{
    for(;*a && *b; ++a, ++b){
        if(tolower(*a) != tolower(*b))
            return 0;
    }
    return *a ? a : b;
}

const char* leading(const char* text)
{
    while(*text && isspace(*text))
        ++text;
    return text;
}


//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Chunk::Chunk(ChunkType t) : type(t), file(nullptr), line(0), mask(0xFFFF), 
    next(nullptr), number(0)
{
}

Chunk*              Chunk::lastSibling() const
{
    Chunk*  c   = (Chunk*) this;
    while(c -> next)
        c   = c -> next;
    return c;
}


//Chunk*  Chunk::lastSub() const
//{
    //if(!sub)
        //return nullptr;
    //Chunk* j = sub;
    //while(j -> next)
        //j   = j -> next;
    //return j;
    
//}

std::string     concatText(Chunk*head, bool fRecurse)
{
    std::string ret;
    for(Chunk*c = head; c ; c = c -> next){
        if(c -> type == ctText)
            ret += c -> text;
        if(fRecurse){
            for(Chunk * a : c->args)
                ret += concatText(a, true);
        }
    }
    return ret;
}

std::string         Chunk::argText(size_t n) const
{
    if(n < args.size()) 
        return concatText(args[n], true);
    return std::string();
}

size_t          count(Chunk*head)
{
    size_t  ret = 0;
    for(Chunk*c = head; c ; ++ret, c = c -> next){
        for(Chunk* a : c -> args)
            ret += count(a);
    }
    return ret;
}

//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

namespace {
   
    uint16_t    decodeSlot(const char*& line)  noexcept(false)
    {
        //  Syntax for slots is:
        //      \*
        //  *or*
        //      \#[+-][,#[+-] ... ]
        //  Where # is 1 to 16
        //
        //  Slots can be followed be followed by white space or "\" or EOL
        
        if(*line == '0'){
            ++line;
            return 0xFFFF;
        } else if(isdigit(*line)){
            uint16_t    mask   = 0;
            do {
                char    a       = *line++;
                unsigned acc    = 0;

                if(a == '0')
                    throw BadSlot();
                
                if(isdigit(*line)){
                    if((a != '1') || (*line > '6'))
                        throw BadSlot();
                    acc = 10 + (*line - '0');
                    ++line;
                } else
                    acc     = a - '0';
                
                --acc;
                if(*line == '+'){
                    ++line;
                    for(unsigned j=acc;j<16;++j)
                        mask |= (1 << j);
                } else if(*line == '-'){
                    ++line;
                    for(unsigned j=0;j<=acc;++j)
                        mask |= (1 << j);
                } else
                    mask |= (1 << acc);
                
                if(*line == ',' && isdigit(line[1]))
                    ++line;
            } while(isdigit(*line));
            return mask;

        } else
            throw BadSlot();
    }

   
    struct Parser {
        std::stack<Chunk*>      m_stack;
        Chunk*                  m_cur;
        Chunk*                  m_head;
        const char*             m_file;
        size_t                  m_line;

        Parser(const char*);
        Chunk*      make(ChunkType t);
        
        
        //  appends if the current chunk type is not NoType
        void        verify(ChunkType t);
        
        void        push(ChunkType);
        void        append(ChunkType);
        void        pop();
        
        void        parse(const char*);
    };
 
 
    Parser::Parser(const char* file) : 
        m_cur(nullptr),
        m_head(nullptr), 
        m_file(file), 
        m_line(0)
    {
        m_head  = m_cur = new Chunk;
        m_head -> type  = ctFile;
        m_head -> text  = file;
        
        m_cur   = make(ctNone);
        m_head -> args.push_back(m_cur);
    }
    
    void        Parser::push(ChunkType ct)
    {
        Chunk*  c   = make(ct);
        m_cur   -> args.push_back(c);
        m_stack.push(m_cur);
        m_cur   = c;
    }
    
    void        Parser::append(ChunkType ct)
    {
        Chunk*  c   = make(ct);
        m_cur -> next   = c;
        m_cur           = c;
    }

    void        Parser::pop()
    {
        if(m_stack.empty())
            throw Syntax("Brace mismatch detected!");
        m_cur       = m_stack.top();
        m_stack.pop();
    }
    
    
    void    Parser::verify(ChunkType t)
    {
        if(m_cur -> type == ctNone){
            m_cur -> type   = t;
            m_cur -> line   = m_line;
        } else if(m_cur -> type != t){
            append(t);
        }
    }
    
    Chunk*  Parser::make(ChunkType t)
    {
        Chunk* c    = new Chunk(t);
        c -> file = m_file;
        c -> line = m_line;
        return c;
    }

    void    Parser::parse(const char* line)
    {
        line        = nospace(line);
        if(!*line){
            //verify(ctNewLine);
            if(m_cur -> type && (m_cur -> type != ctNewLine))
                append(ctNewLine);     // empty line, fine, record it
            return ;
        }
        
        bool            done = false;
        const char*     z       = nullptr;
        const char*     y       = nullptr;
        ChunkType       ct      = ctNone;
        bool            trailing    = false;
        char            last        = '\0';
        bool            first       = true;
        
        for(;*line && !done; last = *line, ++line, first=false){

            switch(*line){
            case '#':
                done    = true;
                break;
            case '\r':
            case '\n':
                if(first)
                    verify(ctNewLine);
                done    = true;
                break;
            case '{':
                append(ctSelect);
                push(ctNone);
                break;
            case '}':
                if(line[1] == '{'){ // next arg
                    pop();
                    push(ctNone);
                    ++line;
                } else
                    pop();
                break;
            case '[':
                switch(line[1]){
                case ':':
                    ct      = ctUmlat;
                    break;
                case '`':
                    ct      = ctGrave;
                    break;
                case '\'':
                    ct      = ctAccent;
                    break;
                case '~':
                    ct      = ctTilde;
                    break;
                case '^':
                    ct      = ctCirc;
                    break;
                case '#':
                    ct      = ctCode;
                    break;
                case '?':
                    ct      = ctChar;
                    break;
                default:
                    throw Syntax("Unreconized Request in character specifier []");
                }
                    
                verify(ct);
                for(z = line; *z && *z != ']'; ++z){
                }
                if(z <= line + 2)
                    throw Syntax("Bad character specifier");
                m_cur -> text  = std::string(line+2, z - line - 2);
                line    = z;
                if(!*line)
                    --line;
                break;
            
            case ']':
                throw Syntax("Stray ']' detected!");
            case '|':
                pop();
                push(ctNone);
                break;
            case '*':
                append(trailing?ctBoldOff:ctBoldOn);
                break;
            case '/':
                append(trailing?ctItalicOff:ctItalicOn);
                break;
            case '-':
                if(line[1] == '-'){
                    ++line;
                    append(ctMDash);
                } else if(isalpha(last) && isalpha(line[1])){
                    append(ctNDash);
                } else {
                    append(trailing?ctStrikeOff:ctStrikeOff);
                }
                break;
            case '^':
                append(trailing?ctSuperOff:ctSuperOn);
                break;
            case '_':
                append(trailing?ctUnderOff:ctUnderOn);
                break;
            case '~':
                append(trailing?ctSubOff:ctSubOn);
                break;
            case '&':
                trailing    = false;
                append(ctAmp);
                break;
            case '<':
                trailing    = false;
                append(ctLess);
                break;
            case '>':
                trailing    = false;
                append(ctGreat);
                break;
            case '"':
                append(trailing?ctRDQuote:ctLDQuote);
                break;
            case '`':
                if(line[1] == '`'){
                    append(ctLDQuote);  // got some old latex conventions....
                    ++line;
                } else
                    append(ctLSQuote);
                break;
            case '\'':
                if(line[1] == '\''){
                    append(ctRDQuote);  // got some old latex conventions....
                    ++line;
                } else if(trailing)
                    append(trailing ? ctRSQuote : ctLSQuote);
                break;
            case '.':
                if(line[1] == '.' && line[2] == '.'){
                    line += 2;
                    append(ctEllipse);
                    break;
                } 
                goto deftext;

            case '@':
                ++line;
                
                if(isdigit(*line)){
                    z               = line;
                    uint16_t  sr    = decodeSlot(line);
                    y               = line;
                    line            = nospace(y);
                    --line;
                    
                    if(first && !*y){
                        append(ctSlotChange);
                    } else if(*y){
                        append(ctSlotNext);
                    } else {
                        throw Syntax("Slot specifier in the wrong location!");
                    }
                    
                    m_cur -> mask   = sr;
                    m_cur -> text   = std::string(line, y-z);
                } else if(ispunct(*line)){
                    m_cur -> text += *line;
                } else if(isalpha(*line)){
                    append(ctCommand);
                    for(z=line;*z && (isalnum(*z) || (*z == '-') || (*z == '_')); ++z)
                        ;
                    m_cur -> text   = std::string(line, z-line);
                    line    = z;
                    if(*z == '{')
                        //  it's an argument....
                        push(ctNone);
                    else
                        --line;
                } else if(isspace(*line)){
                    // It's a text command
                    goto deftext;
                } else 
                    throw Syntax("Unrecognized command syntax!");
                break;
                
            default:
            deftext:
                if(first && (m_cur -> type == ctText))
                    m_cur->text += "  ";
                verify(ctText);
                m_cur -> text    += *line;
                if(isalnum(*line))
                    trailing        = true;
                else if(isspace(*line))
                    trailing        = false;
                break;
            }
            
        }
    }
} 
 
Chunk*    chunkify (FILE*in, const char*fname)
{
    Parser          parser(fname);
    static size_t   bsize       = 1048576;
    static char*    buffer      = (char*) malloc(bsize);
    
    if(gDebug)
        fprintf(stderr, "Chunkifying file %s.\n", fname);
    
    for(parser.m_line = 1; !feof(in);++parser.m_line){
        if(getline(&buffer, &bsize,in)){
            estrip(buffer, bsize);
            try {
                parser.parse(buffer);
            }
            catch(BadSlot)
            {
                fprintf(stderr, "%s %lu: Bad Slot Specifier!\n", fname, parser.m_line);
            }
            catch(Syntax ex)
            {
                fprintf(stderr, "%s %lu: %s\n", fname, parser.m_line, ex.what);
            }
        }
    }
    
    //if(!parser.m_stack.empty())
        //fprintf(stderr, "%s: Imbalance in nested statements!\n", fname);
    
    return parser.m_head;
}

const char* keyFor(ChunkType ct)
{
    switch(ct){
    case ctNone:
        return "(none)";
    case ctFile:
        return "(file)";
    case ctNewLine:
        return "(line)";
    case ctText:
        return "(text)";
    case ctCommand:
        return "(cmd)";
    case ctSlotChange:
        return "(slot chg)";
    case ctSlotNext:
        return "(slot next)";
    case ctSelect:
        return "(select)";
    case ctUmlat:
        return "(umlat)";
    case ctGrave:
        return "(grave)";
    case ctAccent:
        return "(accent)";
    case ctCirc:
        return "(circ)";
    case ctTilde:
        return "(tilde)";
    case ctCode:
        return "(code)";
    case ctChar:
        return "(char)";
    case ctBoldOn:
        return "(bold)";
    case ctBoldOff:
        return "(!bold)";
    case ctItalicOn:
        return "(italic)";
    case ctItalicOff:
        return "(!italic)";
    case ctUnderOn:
        return "(under)";
    case ctUnderOff:
        return "(!under)";
    case ctStrikeOn:
        return "(strike)";
    case ctStrikeOff:
        return "(!strike)";
    case ctSuperOn:
        return "(super)";
    case ctSuperOff:
        return "(!super)";
    case ctSubOn:
        return "(sub)";
    case ctSubOff:
        return "(!sub)";
    case ctAmp:
        return "(amp)";
    case ctGreat:
        return "(great)";
    case ctLess:
        return "(less)";
    case ctLDQuote:
        return "(l-d-quote)";
    case ctRDQuote:
        return "(r-d-quote)";
    case ctLSQuote:
        return "(l-s-quote)";
    case ctRSQuote:
        return "(r-s-quote)";
    case ctSQuote:
        return "(s-quote)";
    case ctDQuote:
        return "(d-quote)";
    case ctNDash:
        return "(n-dash)";
    case ctMDash:
        return "(m-dash)";
    case ctEllipse:
        return "(...)";
    default:
        return "(\?\?\?)";
    }
}


//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

static void        writeChunk(FILE*out, const Chunk* chunk, unsigned int depth)
{
    char    pad[161];
    
    
    unsigned int N  = depth << 1;
    if(N >= sizeof(pad))
        N   = sizeof(pad) - 1;
    
    for(unsigned int i=0;i<N;++i)
        pad[i]  = ' ';
    pad[N]  = '\0';
    
    for(;chunk;chunk = chunk -> next){
        fputs(pad, out);
        fputs(keyFor(chunk -> type), out);
        fprintf(out, "[%d]", chunk -> line);
        if(!chunk -> text.empty()){
            putc(' ', out);
            fputs(chunk->text.c_str(), out);
        }
        putc('\n', out);
        for(size_t i=0;i<chunk->args.size();++i){
            fprintf(out, "%s[%lu]:\n", pad, (i+1));
            writeChunk(out, chunk->args[i], depth+1);
        }
    }
}

void        print(FILE*out, const Chunk* chunk)
{
    writeChunk(out, chunk, 0);
}

//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

const char* chunkTypeName(ChunkType ct)
{
    switch(ct){
    case ctNone:
        return "None";
    case ctFile:
        return "File";
    case ctNewLine:
        return "NewLine";
    case ctText:
        return "Text";
    case ctCommand:
        return "Command";
    case ctSlotChange:
        return "SlotChange";
    case ctSlotNext:
        return "SlotNext";
    case ctSelect:
        return "Select";
    case ctUmlat:
        return "Umlat";
    case ctGrave:
        return "Grave";
    case ctAccent:
        return "Accent";
    case ctCirc:
        return "Circ";
    case ctTilde:
        return "Tilde";
    case ctCode:
        return "Code";
    case ctChar:
        return "Char";
    case ctBoldOn:
        return "BoldOn";
    case ctBoldOff:
        return "BoldOff";
    case ctItalicOn:
        return "ItalicOn";
    case ctItalicOff:
        return "ItalicOff";
    case ctUnderOn:
        return "UnderOn";
    case ctUnderOff:
        return "UnderOff";
    case ctStrikeOn:
        return "StrikeOn";
    case ctStrikeOff:
        return "StrikeOff";
    case ctSuperOn:
        return "SuperOn";
    case ctSuperOff:
        return "SuperOff";
    case ctSubOn:
        return "SubOn";
    case ctSubOff:
        return "SubOff";
    case ctAmp:
        return "Amp";
    case ctGreat:
        return "Great";
    case ctLess:
        return "Less";
    case ctLDQuote:
        return "LDQuote";
    case ctRDQuote:
        return "RDQuote";
    case ctLSQuote:
        return "LSQuote";
    case ctRSQuote:
        return "RSQuote";
    case ctSQuote:
        return "SQuote";
    case ctDQuote:
        return "DQuote";
    case ctNDash:
        return "NDash";
    case ctMDash:
        return "MDash";
    case ctEllipse:
        return "Ellipse";
    default:
        return "Unrecognized";
    }
}


namespace {
    
}

const char*     pad(int n)
{
    static const char  ret[] = "                                                                 ";
    n   = sizeof(ret) - 2*n - 1;
    if(n<8)
        n   = 0;
    if(n >= (int) sizeof(ret))
        n   = sizeof(ret) - 1;
    return ret + n;
}

Chunk*    selectSlot(Chunk* head, uint8_t slot)
{
    uint16_t    mask    = 0xFFFF;

    Chunk*      ret     = nullptr;
    Chunk*      cur     = nullptr;
    Chunk*      ptr     = nullptr;
    Chunk*      nxt     = nullptr;
    
    for(cur = head; cur; cur = cur -> next){
        switch(cur -> type){
        case ctSlotNext:
            if(cur -> next)    // If this fails, there is nothing next...
                if(!(cur -> mask & (1 << slot)))    // skip the next item
                    cur = cur -> next;
            break;
        case ctSlotChange:
            mask        = cur -> mask;
            break;
        case ctSelect:
            // In this case, we select....
            if(!cur->args.empty()){
                if(slot < cur->args.size())
                    nxt     = cur->args[ slot ];
                else
                    nxt     = cur->args.back();
                
                if(ptr)
                    ptr -> next = nxt;
                else
                    ret     = nxt;
                    
                ptr         = nxt -> lastSibling();
            } 
            break;
        
        default:
            //  This is the ordinary case
            if(mask & (1 << slot)){ // append
                if(ptr)
                    ptr -> next = cur;
                else
                    ret         = cur;
                ptr             = cur;
                
                for(Chunk*& c : cur -> args){  // and, recurse....
                    c       = selectSlot(c, slot);
                }
            }
            break;
        }
    }
    
    ptr -> next = nullptr;
    
    return ret;
}
