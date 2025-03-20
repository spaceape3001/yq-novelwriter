////////////////////////////////////////////////////////////////////////////////
//
//  YOUR QUILL
//
////////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <stack>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <set>
#include <vector>
#include "common.hpp"



//  ============================================================================
//  Utilities............
//  ============================================================================



//  ============================================================================
//  Write control
//  ============================================================================


bool            gDebug      = false;

//struct Control {
    
    //FILE*                       file;

    //Control() : file(stdout)
    //{
    //}
    
    //~Control() { close(); }
    
    //void    close()
    //{
        //if(file && (file != stdout) && (file != stderr) && (file != stdin))
            //fclose(file);
        //file    = nullptr;
    //}
    
    //bool    changeFile(const char* name)
    //{
        //close();
        //file    = fopen(name, "w");
        //return file ? true : false;
    //}
//};

//Control                 gWrite;

//  ============================================================================
//  File Parser/Interpreter
//  ============================================================================

typedef Formatter*  (*FNFormatCreator)();

struct FormatSpec {
    const char*         name;
    FNFormatCreator     fnFormat;
    OutputMode          defOutput;
    bool                defSlot;
    const char*         desc;
} kFormats[] = {
    { "null", nullptr, omNone, false, "NULL output." },
    { "debug", fmtDebug, omFull, false, "Debug output, verifies parsing" },
    { "text", fmtText, omFull, true, "Simple Text" },
    { "latex", fmtLatex, omFull, true, "Latex" },
    { "markdown", fmtMarkdown, omFull, true, "Markdown" },
    { "html", fmtHTML, omFull, true, "HTML" },
    { "aff", fmtAFF, omChapter, true, "Adult-Fanfiction.org" },
    { "ao3", fmtAO3, omChapter, true, "Archive of Our Own" },
    { "ffn", fmtFFN, omChapter, true, "Fanfiction.net" },
    { "rtf", fmtRTF, omChapter, true, "Rich Text Format" }
    
};

FormatSpec* findFormat(const char* z)
{
    if(isdigit(*z)){
        unsigned u      = atoi(z);
        if(u < sizeof(kFormats) / sizeof(FormatSpec))
            return &kFormats[u];
        return nullptr;
    }
    
    for(FormatSpec& f : kFormats){
        if(equal(f.name, z))
            return &f;
    }
    
    return nullptr;
}


void print_help(const char* arg0)
{
    const char* szHelp = 
   "Usage: %s [options] [file1...N]\n"
   "    -h          Help (this)\n"
   "    -v          Verbose (when available)\n"
   "    -1..16      Choose slot (default 1)\n"
   "    -f <fmt>    Format\n"
   "    -c <#>      Chapter number/name\n"
   "    -p <dst>    Prefix, default none, must be after format\n"
   "    -m <mode>   Output mode, must be after format,\n"
   "                    full  := full output\n"
   "                    chap  := by chapter\n"
   "                    sect  := by section\n"
   "    -y          Enables slot selection (default), must be after format\n"
   "    -n          Disables slot selection, must be after format\n"
   "\n"
   "Formats:\n"
    ;
    
    fprintf(stderr, szHelp, arg0);
    
    unsigned u  = 0;
    for(auto& k : kFormats){
        fprintf(stderr, "%u %10s %s\n", u, k.name, k.desc);
        ++u;
    }
}


//  ============================================================================
//  Main loop, examines the command line arguments, applies them as seen
//  ============================================================================


int main(int argc, char* argv[])
{
    try {
        FILE*           in      = 0;
        bool            any     = false;
        int             i;
        size_t              na, nb;

        Formatter*          fmt     = nullptr;
        Opt<OutputMode>     omMode(omNone);
        Opt<bool>           fSlot(true);
        Opt<uint8_t>        slot(0);
        Opt<const char*>    prefix(nullptr);
        Opt<const char*>    chap(nullptr);
        Opt<const char*>    sect(nullptr);
        Opt<bool>           toc(false), hyper(false), names(false), css(false);

        Chunk*              head    = nullptr;
        
        auto readThis = [&](FILE* file, const char* path)
        {
            static Chunk*   cur = nullptr;
            Chunk*          c   = chunkify(file, path);
            if(c){
                if(!head){
                    head   = cur   = c;
                } else {
                    cur -> next = c ;
                    cur         = c;
                }
            } else if(gDebug){
                fprintf(stderr, "Warning, nothing was chunked from '%s'\n", path);
            }
        };
        
        for(i=1;i<argc;++i){
            if(argv[i][0] == '-'){
                switch(argv[i][1]){
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    if(isdigit(argv[i][1])){ // it's a slot
                        unsigned a      = 0;
                        a = atoi(argv[i]+1);
                        if(!a || a > 16)
                            throw Bad("Slot out of range, must be 1..16");
                        --a;
                        slot        = a;
                        fSlot       = true;
                    }
                    continue;
                case 'p':
                case 'P':
                    // assumes the directory, if part of the path, is valid... 
                    if(i >= (argc-1))
                        throw Bad("Prefix requires an argument!");
                    ++i;
                    prefix       = argv[i];
                    break;
                case 'f':
                case 'F':
                    if(fmt)
                        throw Bad("Format already specified.");
                    if(i >= (argc-1))
                        throw Bad("Format requires an argument!");
                    ++i;
                    
                    {
                        FormatSpec* fs  = findFormat(argv[i]);
                        if(!fs)
                            throw Bad("Format specifier not recognized, either a name or number required");
                        fmt     = fs -> fnFormat();
                        if(!fmt)
                            throw Bad("Cannot create the format requested (this is a developer error)");
                        if(!omMode.set)
                            omMode.value    = fs -> defOutput;
                        if(!fSlot.set)
                            fSlot.value     = fs -> defSlot;
                    }
                    break;
                case 'c':
                case 'C':
                    if(i >= (argc-1))
                        throw Bad("Chapter requires an argument.");
                    ++i;
                    chap        = argv[i];
                    break;
                case 'h':
                case 'H':
                    throw Help();
                case 's':
                case 'S':
                    if(i >= (argc-1))
                        throw Bad("Section requires an argument.");
                    ++i;
                    sect    = argv[i];
                    break;
                case 'v':
                case 'V':
                    if(gDebug)
                        gTrace   = true;
                    else
                        gDebug   = true;
                    continue;
                case 'y':
                case 'Y':
                    fSlot   = true;
                    break;
                case 'n':
                case 'N':
                    fSlot   = false;
                    break;
                case 'x':
                case 'X':
                    if(i >= (argc-1))
                        throw Bad("Extra parameter requires an argument");
                    ++i;
                    if(equal(argv[i], "toc"))
                        toc     = true;
                    if(equal(argv[i], "hyper"))
                        hyper   = true;
                    if(equal(argv[i], "names"))
                        names   = true;
                    if(equal(argv[i], "css"))
                        css     = true;
                    break;
                case 'm':
                case 'M':
                    if(i >= (argc-1))
                        throw Bad("Mode requires an argument");
                    ++i;
                    if(equal(argv[i], "full"))
                        omMode = omFull;
                    else if(equal(argv[i], "chap"))
                        omMode = omChapter;
                    else if(equal(argv[i], "sect"))
                        omMode = omSection;
                    else
                        throw Bad("Output mode not recognized." );
                    break;
                default:
                    throw Bad("Flag '%s' not recognized.", argv[i]);
                }
            } else {
                in  = fopen(argv[i], "r");
                if(!in)
                    throw Bad("Unable to read the file '%s'", argv[i]);
                if(gDebug)
                    na      = count(head);
                readThis(in, argv[i]);
                fclose(in);
                if(gDebug){
                    nb      = count(head);
                    fprintf(stderr, "File '%s' contributed %lu chunks\n", argv[i], (nb-na));
                }
                in      = 0;
                any = true;
            }
        }
        
        if(!any)
            readThis(stdin, "(stdin)");
            
        if(head && fSlot.value)
            head        = selectSlot(head, slot.value);
        if(!head)
            throw Bad("Either no input or nothing survived slot selection, aborting.");
        
        if(!fmt)
            fmt     = fmtDebug();
            
        for(FormatSpec& fs : kFormats)
            if(fs.defSlot)
                Formatter::allFormats.insert(fs.name);
            
        fmt -> sPrefix      = prefix.value;
        fmt -> eOutMode     = omMode.value;
        fmt -> bTOC         = toc.value;
        fmt -> bHyperlink   = hyper.value;
        fmt -> bNames       = names.value;
        fmt -> bCSS         = css.value;

        fmt -> process(head);
        fmt -> finalize();
        return 0;
    }
    catch(Help)
    {
        print_help(argv[0]);
        return 0;
    }
    catch(Bad ex)
    {
        if(ex.arg)
            fprintf(stderr, ex.what, ex.arg);
        else
            fprintf(stderr, ex.what);
        fputc('\n', stderr);
        return -1;
    }
    catch(BadWrite ex)
    {
        fprintf(stderr, "Unable to open the file '%s' for writing.\n", 
            ex.file.c_str());
    }
}

