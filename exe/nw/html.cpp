////////////////////////////////////////////////////////////////////////////////
//
//  YOUR QUILL
//
////////////////////////////////////////////////////////////////////////////////

#include "common.hpp"
#include <libgen.h>
#include <time.h>
#include <algorithm>
#include <iostream>

struct Html : public Formatter {
    
    struct BA {
        const char* before;
        const char* after;
        
        BA() {}
        BA(const char* _b, const char* _a) : before(_b), after(_a) {}
    };
    
        
    bool                divSpan, altTable, altBlockQuote;   // sub-class config
    bool                inPar, nxtPar, bFirst, bAnyText, bWriteTime;
    std::string         css, cssfull;
    std::vector<Chunk*> footnotes;
    std::string         theTime;
    Style*              chapter;
    
    BA                  baStoryTitle, baStoryAuthor, baChapter, baFootnote;
    const char*         sFootnoteColon;
    bool                bDocTitle;
    bool                inFootnotes;
    std::stack<Align>   align;
    
    Html() : 
        divSpan(true), altTable(false), altBlockQuote(false), 
        inPar(false), nxtPar(false), bFirst(true), bAnyText(false), bWriteTime(false),
        chapter(nullptr),
        baStoryTitle("<CENTER><H1><B>", "</B></H1></CENTER>\n"),
        baStoryAuthor("<CENTER><I>", "</I></CENTER><BR>\n"),
        baChapter("<CENTER><H2><STRONG>", "</STRONG></H2></CENTER>\n"),
        baFootnote("<SUP>", "</SUP>"),
        sFootnoteColon(""),
        bDocTitle(true),
        inFootnotes(false)
    {
        sSuffix = ".html";
        formats.insert("html");
        
        time_t now;
        time(&now);
        theTime =   ctime(&now);
    }
    
    virtual ~Html()
    {
    }

    void    par()
    {
        if(nxtPar && !inPar){
            if(!bCSS){
                switch(curAlign()){
                case AlignRight:
                    fputs("<p style=\"text-align:right\">", out);
                    break;
                case AlignCenter:
                case AlignLeft:
                case AlignNone:
                default:
                    fputs("<p>", out);
                    break;
                }
            }
            fputs("<p>", out);
            inPar   = true;
            nxtPar  = false;
        }
    }
    
    void    endPar()
    {
        if(inPar){
            fputs("</p>\n", out);
            inPar       = false;
            nxtPar      = false;
        }
    }
    
    Align   curAlign() const 
    {
        if(align.empty())
            return AlignNone;
        return align.top();
    }
    
    void    prepForRender() override
    {
        if(bCSS){
            char    filename[1024];
            sprintf(filename, "%s.css", sPrefix);
            cssfull = filename;
            css     = basename(filename);
        }
        
        auto itr    = styles.find("chapter");
        if(itr != styles.end())
            chapter = itr -> second;

        for(auto& i : styles){
            Style*  sty      = i.second;
            if(!sty)
                continue;
                
            if(sty -> align != AlignNone)
                sty -> bForceNewline        = true;
            
            if(divSpan){
                if(sty -> bBlock || sty->bForceNewline)
                    sty -> tag = tsDiv;
                else 
                    sty -> tag = tsSpan;
                sty -> bCss      = bCSS;
            } else if(sty -> bBlock){
                if(altTable){
                    sty -> tag     = tsTable;
                    sty -> bCss      = bCSS;
                } else if(altBlockQuote){
                    sty -> tag      = tsBlock;
                    sty -> bCss      = bCSS;
                } else {
                    sty -> tag      = tsFake;
                    sty -> bCss      = false;
                }
            } else {
                sty -> tag      = tsNone;
                sty -> bCss     = false;
            }
        }
    }
    
    void        cssStyle(Style* sty)
    {
        if(equal(sty -> chunk -> text, "body")){
            fputs("BODY {\n", out);
        } else {
            switch(sty->tag){
            case tsDiv:
                fprintf(out, "DIV.%s {\n", sty -> chunk -> text.c_str());
                break;
            case tsSpan:
                fprintf(out, "SPAN.%s {\n", sty -> chunk -> text.c_str());
                break;
            default:
                return;
            }
        }
        
        switch(sty -> align){
        case AlignLeft:
            fputs("    text-align:left;\n", out);
            break;
        case AlignCenter:
            fputs("    text-align:center;\n", out);
            break;
        case AlignRight:
            fputs("    text-align:right;\n", out);
            break;
        default:
            break;
        }
        
        //if(sty -> indent && (sty -> tag == tsDiv)){
            //int     amt = 2 * sty -> indent;
            //fprintf(out, "    padding-left:%dem;\n"
                         //"    padding-right:%dem;\n", amt, amt);
        //}
        if(!sty->padLeft.empty())
            fprintf(out, "    padding-left: %s;\n", sty -> padLeft.c_str());
        if(!sty->padRight.empty())
            fprintf(out, "    padding-right: %s;\n", sty -> padRight.c_str());
        if(!sty->padTop.empty())
            fprintf(out, "    padding-top: %s;\n", sty -> padTop.c_str());
        if(!sty->padBottom.empty())
            fprintf(out, "    padding-bottom: %s;\n", sty -> padBottom.c_str());
        
        if(!sty -> color.empty())
            fprintf(out, "    color: %s;\n", sty -> color.c_str());
        
        if(!sty -> bgColor.empty() && (sty->tag == tsDiv))
            fprintf(out, "    background-color: %s;\n", sty -> bgColor.c_str());
        
        if(sty -> bBold)
            fputs("    font-weight: bold;\n", out);
        if(sty -> bItalic)
            fputs("    font-style: italic;\n", out);
        if(sty -> bSuperscript)
            fputs("    vertical-align: super;\n", out);
        if(sty -> bSubscript)
            fputs("    vertical-align: sub;\n", out);
        if(sty -> bSmallCaps)
            fputs("    font-variant: small-caps;\n", out);
        if(sty -> bUnderline || sty -> bStrikeThru){
            fputs("    text-decoration:", out);
            if(sty->bUnderline)
                fputs(" underline", out);
            if(sty->bStrikeThru)
                fputs(" line-through", out);
            fputs(";\n", out);
        }
        if(!sty->manual.empty())
            fputs(sty->manual.c_str(), out);
        if(!sty->fontSize.empty())
            fprintf(out, "    font-size: %s;\n", sty->fontSize.c_str());
        if(!sty->fontFamily.empty())
            fprintf(out, "    font-family: \"%s\";\n", sty->fontFamily.c_str());
        
        fputs("}\n", out);
    }
    
    void        process(Chunk* c) override
    {
        Formatter::process(c);
        if(bCSS){
            out     = fopen(cssfull.c_str(), "w");
            if(!out)
                throw BadWrite(cssfull);
            
            auto itr    = styles.find("body");
            if((itr != styles.end()) && itr->second)
                cssStyle(itr->second);
            
            for(auto& i : styles){
                Style*  sty  = i.second;
                if(!sty)
                    continue;
                if(equal(i.first, "body"))
                    continue;
                cssStyle(sty);
            }
            
            
            fflush(out);
            fclose(out);
            out = nullptr;
        }
    }
    
    //  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
    void    writeAccent(const std::string& s) override
    {
        for(char ch : s)
            switch(ch){
            case 'a':
                fputs("&aacute;", out);
                break;
            case 'A':
                fputs("&Aacute;", out);
                break;
            case 'e':
                fputs("&eacute;", out);
                break;
            case 'E':
                fputs("&Eacute;", out);
                break;
            case 'i':
                fputs("&iacute;", out);
                break;
            case 'I':
                fputs("&Iacute;", out);
                break;
            case 'o':
                fputs("&oacute;", out);
                break;
            case 'O':
                fputs("&Oacute;", out);
                break;
            case 'u':
                fputs("&uacute;", out);
                break;
            case 'U':
                fputs("&Uacute;", out);
                break;
            }
    }
    
    void    writeAmpersand() override
    {
        par();
        fputs("&amp;", out);
    }

    void    writeBoldOff() override
    {
        fputs("</b>", out);
    }

    void    writeBoldOn() override
    {
        par();
        fputs("<b>", out);
    }
    
    void    writeBreak() override
    {
        par();
        endPar();
    }
    
    void    writeChapter(Chunk*chk)  override
    {
        endPar();
        nxtPar  = false;
        if(!chk ->args.empty()){
            fputs("<hr>\n", out);
            fputs(baChapter.before, out);
            Chunk*  ch  = textChunk("chapter");
            if(ch)
                render(ch);
            else
                fputs("Chapter", out);
            
            fprintf(out, " %u: ", chk -> number);
            render(chk->args[0]);
            fputs(baChapter.after, out);
        }
        nxtPar  = true;
    }

    void    writeCharacter(const std::string&ch) override
    {
        par();
        fprintf(out, "&%s;", ch.c_str());
    }
    
    void    writeCirc(const std::string& s) override
    {
        for(char ch : s)
            switch(ch){
            case 'a':
                fputs("&acirc;", out);
                break;
            case 'A':
                fputs("&Acirc;", out);
                break;
            case 'e':
                fputs("&ecirc;", out);
                break;
            case 'E':
                fputs("&Ecirc;", out);
                break;
            case 'i':
                fputs("&icirc;", out);
                break;
            case 'I':
                fputs("&Icirc;", out);
                break;
            case 'o':
                fputs("&ocirc;", out);
                break;
            case 'O':
                fputs("&Ocirc;", out);
                break;
            case 'u':
                fputs("&ucirc;", out);
                break;
            case 'U':
                fputs("&Ucirc;", out);
                break;
            }
    }

    void    writeCite(Chunk*c) 
    {
        size_t      N   = 0;
        auto itr = std::find(footnotes.begin(), footnotes.end(), c);
        if(itr != footnotes.end()){
            N   = itr - footnotes.begin() + 1;
        } else {
            footnotes.push_back(c);
            N   = footnotes.size();
        }
        fprintf(out, "%s%d%s", baFootnote.before, (unsigned) N, baFootnote.after);
    }

    void    writeDocStart() override
    {
        Chapter*    ch  = nullptr;
        if((eOutMode >= omChapter) && !chapters.empty()){
            if(curChap >= chapters.size())
                ch      = chapters.back();
            else if(curChap <= 1)
                ch      = chapters.front();
            else
                ch      = chapters[curChap-1];
        }
        Section*    sc  = nullptr;
        if(ch && (eOutMode == omSection) && !ch -> sections.empty()){
            if(curSect >= ch -> sections.size())
                sc      = ch -> sections.back();
            else if(curSect <= 1)
                sc      = ch -> sections.front();
            else
                sc      = ch -> sections[curSect-1];
        }


        fputs("<!DOCTYPE html>\n", out);
        fputs("<HTML>\n", out);
        fputs("<HEAD>\n", out);
        
        fputs("<TITLE>", out);
        if(story && !story->args.empty()){
            render(story->args[0]);
        } else {
            fputs("Untitled", out);
        }

        if(ch && !ch->chunk->args.empty()){
            fprintf(out, ": %u &ndash; ", ch -> chunk -> number);
            render(ch->chunk->args[0]);
        }
        fputs("</TITLE>\n", out);
        
        if(bCSS)
            fprintf(out, "<link rel=\"stylesheet\" type=\"text/css\" href=\"%s\" />\n", css.c_str());
        fputs("</HEAD>\n", out);
        fputs("<BODY>\n", out);
        
        if(story && bDocTitle){
            if((story->args.size() >= 1) && story->args[0]){
                fputs(baStoryTitle.before, out);
                render(story->args[0]);
                fputs(baStoryTitle.after, out);
            }
            if((story->args.size() >= 2) && story->args[1]){
                fputs(baStoryAuthor.before, out);
                render(story->args[1]);
                fputs(baStoryAuthor.after, out);
            }
        }
    }
    
    void    writeDocEnd() override
    {
        endPar();
        if(!footnotes.empty()){
            inFootnotes = true;
            fputs("<hr>\n", out);
            for(size_t i=0;i<footnotes.size();++i){
                inPar   = false;
                nxtPar  = false;
                fprintf(out, "%s%d%s%s ", baFootnote.before, (unsigned) (i+1), 
                    baFootnote.after, sFootnoteColon);
                renderSubs(footnotes[i]);
                fputs("<br>\n", out);
            }
            footnotes.clear();
            inFootnotes = false;
        }
        if(bWriteTime){
            fprintf(out, "<hr>\n<b>Date:</b> <i>%s</i><br>\n", theTime.c_str());
        }

        fprintf(out, "</BODY>\n"
                     "</HTML>\n"
        );
    }
    
    void    writeDQuote() override
    {
        par();
        fputc('"', out);
    }
    
    void    writeEllipse() override
    {
        par();
        fputs("&hellip;", out);
    }

    void    writeFootnote(Chunk*c) 
    {
        footnotes.push_back(c);
        fprintf(out, "%s%d%s", baFootnote.before, (unsigned) footnotes.size(), baFootnote.after);
    }

    void    writeGrave(const std::string& s) override
    {
        par();
        for(char ch : s)
            switch(ch){
            case 'a':
                fputs("&agrave;", out);
                break;
            case 'A':
                fputs("&Agrave;", out);
                break;
            case 'e':
                fputs("&egrave;", out);
                break;
            case 'E':
                fputs("&Egrave;", out);
                break;
            case 'i':
                fputs("&igrave;", out);
                break;
            case 'I':
                fputs("&Igrave;", out);
                break;
            case 'o':
                fputs("&ograve;", out);
                break;
            case 'O':
                fputs("&Ograve;", out);
                break;
            case 'u':
                fputs("&ugrave;", out);
                break;
            case 'U':
                fputs("&Ugrave;", out);
                break;
            }
    }

    void    writeGreater() override 
    {
        par();
        fputs("&gt;", out);
    }
    
    void    writeItalicOff() override
    {
        fputs("</i>", out);
    }


    void    writeItalicOn() override
    {
        par();
        fputs("<i>", out);
    }
    
    void    writeLDQuote() override
    {
        par();
        fputs("&ldquo;", out);
    }

    void    writeLesser() override 
    {
        par();
        fputs("&lt;", out);
    }
    
    void    writeLSQuote() override
    {
        par();
        fputs("&lsquo;", out);
    }
    
    void    writeMDash() override
    {
        par();
        fputs("&mdash;", out);
    }
    
    void    writeNDash() override
    {
        par();
        fputs("&ndash;", out);
    }
    
    void    writeNewline()
    {
        if(inPar)
            fputs("</p>\n", out);
        nxtPar  = true;
        inPar   = false;
    }
    
    void    writeRDQuote() override
    {
        fputs("&rdquo;", out);
    }

    void    writeRSQuote() override
    {
        fputs("&rsquo;", out);
    }
    
    void    writeSQuote() override
    {
        par();
        fputc('\'', out);
    }
    
    void    writeStyleStart(Chunk*, Style&sty) override
    {
        if(sty.bForceNewline || sty.bBlock ){
            bool    wasPar  = inPar;
            if(inPar)
                endPar();
            //if(!bCSS){
            if(sty.bForceNewline){
                fputs("<p></p>\n", out);
            }
            if(sty.bRule)
                fputs("<hr>\n", out);
            //}
        } else {
            if(!inPar && !inFootnotes){
                fputs("<p>", out);
                inPar   = true;
                nxtPar  = false;
            }
        }
        
        switch(sty.tag){
        case tsNone:
            break;
        case tsDiv:
            if(sty.bCss){
                fprintf(out, "<div class=\"%s\">", sty.chunk->text.c_str());
            } else {
                fputs("<div>", out);
            }
            break;
        case tsSpan:
            if(sty.bCss)
                fprintf(out, "<span class=\"%s\">", sty.chunk->text.c_str());
            break;
        case tsBlock:
            fputs("<blockquote>", out);
            break;
        case tsTable:
            fputs("<table border=\"1\"><tr><td>\n", out);
            break;
        case tsFake:
            if(sty.bBorder)
                fputs("<center>=v=v=v=v=v=v=v=v=v=v=v=v=v=v=v=</center><br>", out);
            else
                fputs("<br>", out);
            break;
        }
        
        if(!sty.bCss){
            if(sty.align == AlignCenter)
                fputs("<center>", out);
            if(sty.bBold)
                fputs("<b>", out);
            if(sty.bItalic)
                fputs("<i>", out);
            if(sty.bUnderline)
                fputs("<u>", out);
            if(sty.bSuperscript)
                fputs("<sup>", out);
            if(sty.bSubscript)
                fputs("<sub>", out);
        }
        
        align.push(sty.align);
    }
    
    void    writeStyleEnd(Chunk*, Style&sty) override
    {
        if(!sty.bCss){
            if(sty.bSubscript)
                fputs("</sub>", out);
            if(sty.bSuperscript)
                fputs("</sup>", out);
            if(sty.bUnderline)
                fputs("</u>", out);
            if(sty.bItalic)
                fputs("</i>", out);
            if(sty.bBold)
                fputs("</b>", out);
            if(sty.align == AlignCenter)
                fputs("</center>", out);
        }
        
        if(sty.bBlock && inPar){
            endPar();
            nxtPar  = true;
        }
        
        switch(sty.tag){
        case tsNone:
            break;
        case tsDiv:
            fputs("</div>", out);
            inPar   = false;
            break;
        case tsSpan:
            fputs("</span>", out);
            break;
        case tsBlock:
            fputs("</blockquote>", out);
            inPar   = false;
            break;
        case tsTable:
            fputs("</table>", out);
            inPar   = false;
            break;
        case tsFake:
            if(sty.bBorder)
                //fputs("<center>=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=</center><br>", out);
                fputs("<center>=^=^=^=^=^=^=^=^=^=^=^=^=^=^=^=</center><br>", out);
            else
                fputs("<br>", out);
            break;
        }
        
        if(!align.empty())
            align.pop();
    }

    virtual void    writeSubOff() override
    {
        fputs("</sub>", out);
    }

    virtual void    writeSubOn() override
    {
        par();
        fputs("<sub>", out);
    }
    
    virtual void    writeSuperOff() override
    {
        fputs("</sup>", out);
    }

    virtual void    writeSuperOn() override
    {
        par();
        fputs("<sup>", out);
    }
    

    void    writeText(const std::string& text) override
    {
        par();
        fputs(text.c_str(), out);
    }
    
    void    writeUmlat(const std::string& s) override
    {
        par();
        for(char ch : s)
            switch(ch){
            case 'a':
                fputs("&auml;", out);
                break;
            case 'A':
                fputs("&Auml;", out);
                break;
            case 'e':
                fputs("&euml;", out);
                break;
            case 'E':
                fputs("&Euml;", out);
                break;
            case 'i':
                fputs("&iuml;", out);
                break;
            case 'I':
                fputs("&Iuml;", out);
                break;
            case 'o':
                fputs("&ouml;", out);
                break;
            case 'O':
                fputs("&Ouml;", out);
                break;
            case 'u':
                fputs("&uuml;", out);
                break;
            case 'U':
                fputs("&Uuml;", out);
                break;
            }
    }


    void    writeTilde(const std::string& s) override
    {
        for(char ch : s)
            switch(ch){
            case 'a':
                fputs("&atilde;", out);
                break;
            case 'A':
                fputs("&Atilde;", out);
                break;
            case 'e':
                fputs("&etilde;", out);
                break;
            case 'E':
                fputs("&Etilde;", out);
                break;
            case 'i':
                fputs("&itilde;", out);
                break;
            case 'I':
                fputs("&Itilde;", out);
                break;
            case 'o':
                fputs("&otilde;", out);
                break;
            case 'O':
                fputs("&Otilde;", out);
                break;
            case 'u':
                fputs("&utilde;", out);
                break;
            case 'U':
                fputs("&Utilde;", out);
                break;
            }
    }
    
    void    writeUnderOff() override
    {
        fputs("</u>", out);
    }


    void    writeUnderOn() override
    {
        par();
        fputs("<u>", out);
    }


    void    writeTableStart() 
    {
            // styles TODO
        par();
        fputs("<table>", out);
    }
    
    void    writeTableEnd() 
    {
        fputs("</table>\n", out);
    }
    void    writeTRowStart() 
    {
        fputs("<tr>", out);
    }
    void    writeTRowEnd() 
    {
        fputs("</tr>\n", out);
    }
    void    writeTColStart() override 
    {
        fputs("<td>", out);
    }
    
    void    writeTColEnd() override 
    {
        fputs("</td>", out);
    }

    void    writeTHdrStart() override
    {
        fputs("<th>", out);
    }
    
    void    writeTHdrEnd()
    {
        fputs("</th>", out);
    }

    void    writeItemStart() override
    {
        par();
        fputs("<li>", out);
    }
    
    void    writeItemEnd()  override
    {
        fputs("</li>", out);
    }

    void    writeUListStart() override
    {
        par();
        fputs("<ul>", out);
    }
    
    void    writeUListEnd() override
    {
        fputs("</ul>", out);
    }

    void    writeOListStart() override
    {
        par();
        fputs("<ol>", out);
    }
    
    void    writeOListEnd() override
    {
        fputs("</ol>", out);
    }
};

Formatter*  fmtHTML()
{
    return new Html;
}

//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//  AFF
//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

class AdultFanfiction : public Html {
public:

    AdultFanfiction()
    {
        formats.insert("aff");
    }
    
    virtual ~AdultFanfiction()
    {
    }
    
};


Formatter*  fmtAFF()
{
    return new AdultFanfiction;
}

//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//  AO3
//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

class ArchiveOfOurOwn : public Html {
public:

    ArchiveOfOurOwn()
    {
        formats.insert("ao3");
        bDocTitle       = false;
    }
    
    virtual ~ArchiveOfOurOwn()
    {
    }
    
    void    writeChapter(Chunk*chk)  override
    {
        endPar();
        nxtPar  = false;
        if(chk->number <= 1){
           fputs("<hr>", out);
           if(story && !story->args.empty()){
               fputs("<div class=\"story\">", out);
               render(story->args[0]);
               fputs("</div><hr>\n", out);
           } else 
                fputc('\n', out);
        }
        nxtPar  = true;
    }
};


Formatter*  fmtAO3()
{
    return new ArchiveOfOurOwn;
}


//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//  FFN
//  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

class Fanfiction : public Html {
public:

    Fanfiction()
    {
        formats.insert("ffn");
        divSpan         = false;
        baStoryTitle    = BA("<CENTER><B><LARGE>", "</LARGE></B></CENTER><BR>\n");
        baStoryAuthor   = BA("<CENTER><I>", "</I></CENTER><BR>\n");
        baChapter       = BA("<CENTER><B>", "</B></CENTER><BR>\n");
        baFootnote      = BA("[","]");
        bWriteTime      = true;
        sFootnoteColon  = ":";
    }
    
    virtual ~Fanfiction()
    {
    }
    
};


Formatter*  fmtFFN()
{
    return new Fanfiction;
}

