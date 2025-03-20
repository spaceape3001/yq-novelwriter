////////////////////////////////////////////////////////////////////////////////
//
//  YOUR QUILL
//
////////////////////////////////////////////////////////////////////////////////

#include "common.hpp"
#include <algorithm>
#include <filesystem>

bool IgCase::operator()(const std::string&a, const std::string&b) const
{
    return lessThan(a.c_str(), b.c_str());
}

std::string     legalFile(const char* a)
{
    std::string     ret;
    bool            space   = false;
    for(; *a; ++a){
        if(isalnum(*a)){
            ret += *a;
            space   = false;
        } else {
            if(!space){
                ret += '_';
                space   = true;
            }
        }
    }
    return ret;
}

//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

namespace {
    static const char*  kReserved[] = {
        "act",
        "break",
        "chapter",
        "footnote",
        "include",
        "macros",
        "olist",
        "part",
        "section",
        "story",
        "styles",
        "table",
        "trow",
        "thdr",
        "ulist"
    };

    std::set<std::string,IgCase> makeReservedSet()
    {
        std::set<std::string,IgCase>    ret;
        for(const char* z : kReserved)
            ret.insert(z);
        return ret;
    }
}


//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Formatter::Macro::Macro(Formatter*, Chunk* c) : chunk(c), definition(nullptr)
{
    if(chunk){
        switch(chunk -> args.size()){
        case 0:
            fprintf(stderr, "Define '%s' has NO arguments, are you sure?  (%s:%ul)\n",
                chunk -> text.c_str(), chunk -> file, chunk -> line);
            break;
        default:
            fprintf(stderr, "Define '%s' has more than two arguments, not supported.  (%s:%ul)\n",
                chunk -> text.c_str(), chunk -> file, chunk -> line);
        case 2:
            definition  = chunk -> args[1];
            for(Chunk* t = chunk -> args[0]; t; t = t -> next){
                if(t -> type == ctCommand)
                    args.push_back(t -> text);
            }
            break;
        case 1:
            definition  = chunk -> args[0];
            break;
        }
    }
}

Formatter::Style::Style(Formatter*f, Chunk* c) : chunk(c), 
    bBold(false), bItalic(false), bStrikeThru(false), bUnderline(false), 
    bSuperscript(false), bSubscript(false), bSmallCaps(false),
    bForceNewline(false), 
    bPrepad(false), bRule(false), bBlock(false), bUnPar(false), bBorder(false),
    align(AlignNone),
    //indent(0), 
    tag(tsNone), bCss(false)
{
    for(Chunk* a : c -> args)
        defStyle(f, a);
}

void    Formatter::Style::defStyle(Formatter*f, Chunk*chk)
{
    for(Chunk* c = chk; c; c = c->next){
        if(c->type != ctCommand)
            continue;
        if(allFormats.find(c->text) != allFormats.end()){
            if(f->formats.find(c->text) != f->formats.end())
                for(Chunk* a : c -> args)
                    defStyle(f,a);
            continue;
        }
        
        if(equal(c->text, "bold"))
            bBold       = true;
        if(equal(c->text, "italic"))
            bItalic     = true;
        if(equal(c->text, "strike"))
            bStrikeThru = true;
        if(equal(c->text, "under"))
            bUnderline      = true;
        if(equal(c->text, "super"))
            bSuperscript    = true;
        if(equal(c->text, "sub"))
            bSubscript      = true;
        if(equal(c->text, "newline"))
            bForceNewline   = true;
        if(equal(c->text, "color"))
            color       = c -> argText(0);
        if(equal(c->text, "align")){
            std::string what        = c -> argText(0);
            if(equal(what, "left"))
                align       = AlignLeft;
            else if(equal(what, "center"))
                align       = AlignCenter;
            else if(equal(what, "right"))
                align       = AlignRight;
            else
                fprintf(stderr, "Unable to understand alignment parameter.  %s:%u\n", c->file, c->line);
        }
        if(equal(c->text, "prepad"))
            bPrepad     = true;
        if(equal(c->text, "rule"))
            bRule       = true;
        if(equal(c->text, "block"))
            bBlock      = true;
        if(equal(c->text, "unpar"))
            bUnPar      = true;
        if(equal(c->text, "smcaps"))
            bSmallCaps  = true;
        if(equal(c->text, "bgcolor"))
            bgColor     = c -> argText(0);
        if(equal(c->text, "bgimage"))
            bgImage     = c -> argText(0);
        //if(equal(c->text, "indent")){
            //std::string     txt = c -> argText(0);
            //if(!txt.empty())
                //indent  = atoi(txt.c_str());
        //}
        if(equal(c->text, "border"))
            bBorder      = true;
        if(equal(c->text, "manual"))
            manual  += c->argText(0);
        if(equal(c->text, "size"))
            fontSize    = c->argText(0);
        if(equal(c->text, "left"))
            padLeft     = c -> argText(0);
        if(equal(c->text, "right"))
            padRight    = c -> argText(0);
        if(equal(c->text, "top"))
            padTop      = c -> argText(0);
        if(equal(c->text, "bottom"))
            padBottom   = c -> argText(0);
        if(equal(c->text, "font"))
            fontFamily  = c -> argText(0);
    }
}


//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

std::set<std::string,IgCase>     Formatter::allFormats;

Formatter::Formatter() : eOutMode(omFull), sPrefix(nullptr), sSuffix(""),
    story(nullptr), maxSecId(0), out(nullptr), 
    digChap(0), digSect(0),
    curChap(0), curSect(0),
    bTOC(false), bHyperlink(false), bNames(false), bCSS(false)
{
}

Formatter::~Formatter ()
{
}

//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

void    Formatter::command(Chunk* cmd, const ArgMap& args)
{
    if(equal(cmd->text, "act")){
        writeAct(cmd);
    } else if(equal(cmd->text, "biblio")){
        //  NOT HERE
    } else if(equal(cmd->text, "break")){
        writeBreak();
    } else if(equal(cmd->text, "chapter")){
        switch(eOutMode){
        case omChapter:
            if(cmd -> number > 1){
                endFile();
                curChap     = cmd->number;
                Chapter&    ch  = *chapters[cmd->number - 1];
                out     = fopen(ch.filename.c_str(), "w");
                if(!out)
                    throw BadWrite(ch.filename);
                writeDocStart();
            }
            break;
        case omSection: // for now, don't worry
        default:    
            break;
        }
        
        writeChapter(cmd);
    } else if(equal(cmd->text, "footnote")){
        writeFootnote(cmd);
    } else if(equal(cmd->text, "include")){
        for(Chunk* a : cmd->args)
            render(a, args);
    } else if(equal(cmd->text, "macros")){
        // NOT HERE
    } else if(equal(cmd->text, "olist")){
        writeOListStart();
        for(Chunk* a : cmd -> args){
            writeItemStart();
            render(a, args);
            writeItemEnd();
        }
        writeOListEnd();
    } else if(equal(cmd->text, "part")){
        writePart(cmd);
    //} else if(equal(cmd->text, "scene")){
        //writeScene(cmd);
    } else if(equal(cmd->text, "section")){
        writeSection(cmd);
    } else if(equal(cmd->text, "story")){
        // NOT HERE
    } else if(equal(cmd->text, "styles")){
        // NOT HERE
    } else if(equal(cmd->text, "table")){
        writeTableStart();
        for(Chunk* a : cmd -> args)
            render(a, args);
        writeTableEnd();
    } else if(equal(cmd->text, "thdr")){
        writeTRowStart();
        for(Chunk* a : cmd -> args){
            writeTHdrStart();
            render(a, args);
            writeTHdrEnd();
        }
        writeTRowEnd();
    } else if(equal(cmd->text, "titles")){
        //  DO NOTHING
    } else if(equal(cmd->text, "trow")){
        writeTRowStart();
        for(Chunk* a : cmd -> args){
            writeTColStart();
            render(a, args);
            writeTColEnd();
        }
        writeTRowEnd();
    } else if(equal(cmd->text, "ulist")){
        writeUListStart();
        for(Chunk* a : cmd -> args){
            writeItemStart();
            render(a, args);
            writeItemEnd();
        }
        writeUListEnd();
    } else {
        
        auto def    = macros.find(cmd->text);
        if(def != macros.end()){
            doMacro(cmd, *(def->second), args);
            return ;
        } 
        
        auto sty    = styles.find(cmd -> text);
        if(sty != styles.end()){
            doStyle(cmd, *(sty->second), args);
            return ;
        }
        
        auto fmt        = allFormats.find(cmd -> text);
        if(fmt != allFormats.end()){
            if(formats.find(cmd->text) != formats.end()){
                renderSubs(cmd, args);
            }
            return ;
        }
        
        auto arg        = args.find(cmd -> text);
        if(arg != args.end()){
            if(arg->second)
                render(arg->second, args);
            return ;
        }
        
        auto cite       = biblio.find(cmd -> text);
        if(cite != biblio.end()){
            writeCite(cite->second);
            return ;
        }
        
        
        fprintf(stderr, "'%s' command unrecognized (%s:%u)\n", 
            cmd->text.c_str(), cmd->file, cmd->line);
    }
}

void    Formatter::defBiblio(Chunk* chk)
{
    for(Chunk*  c   = chk; c; c=c->next){
        if((c -> type == ctCommand) && isOkayName(c))
            biblio[c->text]     = c;
    }
}

void    Formatter::defMacros(Chunk*chk)
{
    for(Chunk * c = chk; c; c = c -> next){
        if((c -> type == ctCommand) && isOkayName(c))
            //  we'll check the others here.....
            macros[c->text]    = new Macro(this, c);
    }
}

void    Formatter::defStyles(Chunk*chk)
{
    for(Chunk * c = chk; c; c = c -> next){
        if((c -> type == ctCommand) && isOkayName(c, true))
            //  we'll check the others here.....
            styles[c->text]    = new Style(this, c);
    }
}

void    Formatter::defTitles(Chunk*chk)
{
    for(Chunk * c = chk; c; c = c -> next){
        if(c -> type == ctCommand && !c->args.empty())
            text[c->text]   = c -> args[0];
    }
}

void    Formatter::doMacro(Chunk*c, Macro&mac, const ArgMap& args)
{
    if(mac.args.empty()){
        if(mac.definition)
            render(mac.definition,args);
    } else {
        ArgMap  subargs = args;
        for(size_t i = 0; i<mac.args.size(); ++i){
            if(i >= c->args.size()){
                subargs[mac.args[i]]   = nullptr;
            } else 
                subargs[mac.args[i]]   = c->args[i];
        }
        
        if(mac.definition)
            render(mac.definition, subargs);
    } 
}
    
void    Formatter::doStyle(Chunk*c, Style&sty, const ArgMap& args)
{
    writeStyleStart(c, sty);
    for(Chunk* a : c -> args)
        render(a, args);
    writeStyleEnd(c, sty);
}

void    Formatter::endFile()
{
    if(out){
        //switch(eOutMode){
        //case omFull:
            //writeFootnotes(footnotes);
            //break;
        //case omChapter:
            //if(curChap && (curChap <= chapters.size())){
                //Chapter& ch    = *(chapters[curChap-1]);
                //if(!ch.footnotes.empty())
                    //writeFootnotes(footnotes);
            //}
            //break;
        //case omSection:
        //default:
            ////  TODO
            //break;
        //}
        
        writeDocEnd();
        fflush(out);
        fclose(out);
        out     = nullptr;
    }
}

void    Formatter::enumerate(Chunk* head)
{
    for(Chunk* c = head; c; c = c -> next){
        if(c -> type == ctCommand){
            if(equal(c -> text, "biblio")){
                for(Chunk* a : c -> args)
                    defBiblio(a);
                continue;
            }
            if(equal(c -> text, "act")){
                acts.push_back(c);
                c -> number = acts.size();
            }
            if(equal(c -> text, "include")){
                for(size_t i = 0; i<c->args.size(); ++i){
                    std::string *fname   = new std::string(c -> argText(i));    // need it to persist....
                    if(fname->empty()){
                        fprintf(stderr, "Bad include (%s:%u)\n", c->file, c->line);
                        throw Bad("Empty include specifier!");
                    }

                    std::filesystem::path   cur(c->file);
                    std::filesystem::path   them(*fname);
                    
                    if(them.extension() != "nw")
                        *fname += ".nw";
                    
                    FILE*   in  = fopen(fname->c_str(), "r");
                    if(!in){
                        //  Try filesystem path
                        *fname   = cur.parent_path().string() + "/" + *fname;
                        in  = fopen(fname->c_str(), "r");
                        if(!in){
                            fprintf(stderr, "Bad include statement (%s:%u)\n", c->file, c->line);
                            throw Bad("Unable to readthe file '%s'\n", fname->c_str());
                        }
                    }
                    c->args[i]  = chunkify(in, fname -> c_str());
                    fclose(in);
                    if(!c->args[i])
                        fprintf(stderr, "Warning, nothing was chunked from '%s'\n", fname -> c_str());
                }
            }
            if(equal(c -> text, "part")){
                parts.push_back(c);
                c -> number = parts.size();
            }
            if(equal(c -> text, "chapter")){
                if(c -> args.empty())
                    fprintf(stderr, "Chapter has no name! (%s:%u)\n",
                        c -> file, c -> line );
                        
                chapters.push_back(new Chapter(this, c));
                c -> number = chapters.size();
                auto& ch = *chapters.back();
                if(((eOutMode == omChapter) || (eOutMode == omSection)) && !footnotes.empty())
                    std::swap(footnotes, ch.footnotes);
            }
            if(equal(c -> text, "section")){
                if(c -> args.empty())
                    fprintf(stderr, "Section has no name! (%s:%u)\n",
                        c -> file, c -> line );

                if(chapters.empty()){
                    fprintf(stderr, "Section before chapter (%s:%u)\n",
                        c -> file, c -> line);
                } else {
                    Chapter& ch = *chapters.back();
                    ch.sections.push_back(new Section(this, c));
                    Section& sc    = *ch.sections.back();
                    c -> number = ch.sections.size();
                    if(c -> number > maxSecId)
                        maxSecId    = c -> number;
                    if((eOutMode == omSection) && !ch.footnotes.empty())
                        std::swap(ch.footnotes, sc.footnotes);
                }
            }
            //if(equal(c -> text, "footnote")){
                //switch(eOutMode){
                //case omSection:
                    //if(!chapters.empty()){
                        //Chapter& ch    = *chapters.back();
                        //if(ch.sections.empty()){
                            //Section& sc    = *ch.sections.back();
                            //sc.footnotes.push_back(c);
                            //c -> number = sc.footnotes.size();
                        //} else {
                            //ch.footnotes.push_back(c);
                            //c -> number = ch.footnotes.size();
                        //}
                        //break;
                    //}
                    //break;
                //case omChapter:
                    //if(!chapters.empty()){
                        //Chapter& ch    = *chapters.back();
                        //ch.footnotes.push_back(c);
                        //c -> number = ch.footnotes.size();
                        //break;
                    //}
                    //// fall-through is deliberate
                //case omFull:
                //default:
                    //footnotes.push_back(c);
                    //c -> number = footnotes.size();
                    //break;
                //}
            //}
            if(equal(c -> text, "macros")){
                for(Chunk* a : c -> args)
                    defMacros(a);
                continue;
            }
            if(equal(c -> text, "styles")){
                for(Chunk* a : c -> args)
                    defStyles(a);
                continue;
            }
            if(equal(c -> text, "story")){
                if(story){
                    fprintf(stderr, "Second story title found (%s:%u)\n", 
                        c -> file, c -> line);
                } else
                    story  = c;
            }
            if(equal(c -> text, "titles")){
                for(Chunk* a : c -> args)
                    defTitles(a);
                continue;
            }
        }
        
        for(Chunk* a : c -> args)
            enumerate(a);
    }
}

void    Formatter::finalize()
{
    endFile();
    curChap = curSect   = 0;
}


bool    Formatter::isOkayName(Chunk* c, bool fAllowReserved)
{
    static auto reserved    = makeReservedSet();
    if(!fAllowReserved && (reserved.find(c -> text) != reserved.end())){
        fprintf(stderr, "'%s' is a reserved keyword!  (%s:%u)\n",
            c -> text.c_str(), c -> file, c -> line);
        return false;
    }
        
    auto def        = macros.find(c->text);
    if(def != macros.end()){
        Chunk* dup  = def -> second -> chunk;
        fprintf(stderr, "'%s' is a macro. (%s:%u)\n"
                        "    (first one is at %s:%u)\n",
            c -> text.c_str(), c -> file, c -> line,
            dup -> file, dup -> line);
        return false;
    }
    
    auto sty        = styles.find(c->text);
    if(sty != styles.end()){
        Chunk* dup  = sty -> second -> chunk;
        fprintf(stderr, "'%s' is a block. (%s:%u)\n"
                        "    (first one is at %s:%u)\n",
            c -> text.c_str(), c -> file, c -> line,
            dup -> file, dup -> line);
        return false;
    }
    
    auto fmt        = allFormats.find(c->text);
    if(fmt != allFormats.end()){
        fprintf(stderr, "'%s' is a format. (%s:%u)\n",
            c -> text.c_str(), c -> file, c -> line);
        return false;
    }
    
    auto cite       = biblio.find(c->text);
    if(cite != biblio.end()){
        fprintf(stderr, "'%s' is a bibliographic entry. (%s:%u)\n",
            c -> text.c_str(), c -> file, c -> line);
        return false;
    }
    
    return true;
}

void    Formatter::process(Chunk* c)
{
    if(!sPrefix)
        sPrefix = "nw";

    enumerate(c);
    digChap = digits(chapters.size() + 1);
    digSect = digits(maxSecId);

    char        filename[1024];
    if(!sSuffix)
        sSuffix        = "";
    
    switch(eOutMode){
    case omFull:
        sprintf(filename, "%s%s", sPrefix, sSuffix);
        out     = fopen(filename, "w");
        if(!out)
            throw BadWrite(filename);
        break;
    case omChapter:
    case omSection: //  for now, this is the way....
        for(Chapter* ch : chapters){
            std::string title = ch->chunk->argText(0);
            title       = legalFile(title.c_str());
            if(bNames && !title.empty()){
                sprintf(filename, "%s-%0*u-%s%s", sPrefix, digChap, 
                    ch->chunk->number, title.c_str(), sSuffix);
            } else 
                sprintf(filename, "%s-%0*u%s", sPrefix, digChap, 
                    ch->chunk->number, sSuffix);
            if(!out){
                out     = fopen(filename, "w");
                if(!out)
                    throw BadWrite(filename);
            } else
                ch->filename      = filename;
        }
        break;
    default:  
        break;
    }

    if(!out){
        fprintf(stderr, "Nothing to do, no file was opened!\n");
        return ;
    }
    
    prepForRender();

    writeDocStart();
    render(c);
    endFile();
}

void    Formatter::render(Chunk* head, const ArgMap& args)
{
    for(Chunk* c = head; c; c = c -> next){
        switch(c -> type){
        case ctNewLine:
            writeNewline();
            break;
        case ctText:
            writeText(c->text);
            break;
        case ctCommand:
            command(c, args);
            continue;
        case ctUmlat:
            writeUmlat(c->text);
            break;
        case ctGrave:
            writeGrave(c->text);
            break;
        case ctAccent:
            writeAccent(c->text);
            break;
        case ctCirc:
            writeCirc(c->text);
            break;
        case ctTilde:
            writeTilde(c->text);
            break;
        case ctCode:
            writeCode(c->text);
            break;
        case ctChar:
            writeCharacter(c->text);
            break;
        case ctBoldOn:
            writeBoldOn();
            break;
        case ctBoldOff:
            writeBoldOff();
            break;
        case ctItalicOn:
            writeItalicOn();
            break;
        case ctItalicOff:
            writeItalicOff();
            break;
        case ctUnderOn:
            writeUnderOn();
            break;
        case ctUnderOff:
            writeUnderOff();
            break;
        case ctStrikeOn:
            writeStrikeOn();
            break;
        case ctStrikeOff:
            writeStrikeOff();
            break;
        case ctSuperOn:
            writeSuperOn();
            break;
        case ctSuperOff:
            writeSuperOff();
            break;
        case ctSubOn:
            writeSubOn();
            break;
        case ctSubOff:
            writeSubOff();
            break;
        case ctAmp:
            writeAmpersand();
            break;
        case ctGreat:
            writeGreater();
            break;
        case ctLess:
            writeLesser();
            break;
        case ctLDQuote:
            writeLDQuote();
            break;
        case ctRDQuote:
            writeRDQuote();
            break;
        case ctLSQuote:
            writeLSQuote();
            break;
        case ctRSQuote:
            writeRSQuote();
            break;
        case ctSQuote:
            writeSQuote();
            break;
        case ctDQuote:
            writeDQuote();
            break;
        case ctNDash:
            writeNDash();
            break;
        case ctMDash:
            writeMDash();
            break;
        case ctEllipse:
            writeEllipse();
            break;
        default:
            break;
        }
        renderSubs(c, args);
    }
}

void            Formatter::renderSubs(Chunk*c, const ArgMap& args)
{
    for(Chunk* a : c -> args)
        render(a, args);
}

Chunk*          Formatter::textChunk(const std::string&k)
{
    auto itr    = text.find(k);
    if(itr != text.end())
        return itr -> second;
    return nullptr;
}

//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

struct Debug : public Formatter {
    Debug() {}
    virtual ~Debug(){}
    
    void    process(Chunk* chk) override
    {
        print(stdout, chk);
    }
    
    void    finalize() override
    {
    }
};

Formatter*  fmtDebug()
{
    return new Debug;
}



