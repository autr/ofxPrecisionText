/*
 * ofxPrecisionText
 * Gilbert Sinnott
 * https://github.com/autr/ofxPrecisionText
 *
 * Adapted from:
 *      URL: https://github.com/goldsborough/markdownpp
 *      Date: 30 oct. 2014
 *      Author: cameron
 */

#include <fstream>
#include <iostream>
#include <string>
#include <regex>

#include "parseMarkdown.h"
#include "ofLog.h"
#include "ofUtils.h"

using namespace std;
/**
 * Bool
 */
bool need_paragraph = true;
bool current_paragraph = false;
bool current_list = false;

/**
 * Regex
 */
//*some tex*
regex italic_regex("[<>a-zA-Z0-9 ]*\\*([a-zA-Z0-9 ]*)\\*[<>a-zA-Z0-9 ]*");
//**some text**
regex bold_regex("\\*\\*(.*)\\*\\*");
//*   Some text
regex list_regex("\\*\t(.*)");
//[some text](URL)
regex url_regex("[^!]*\\[(.*)\\]\\((.*)\\)");
//![some text](URL)
regex img_regex("!\\[(.*)\\]\\((.*)\\)");

/*
 * H1, H2 and H3 style headers regexs
 */
//# some text
regex h1_regex("# (.*)");
//## some text
regex h2_regex("## (.*)");
//### some text
regex h3_regex("### (.*)");

extern bool need_paragraph;
extern bool current_paragraph;
extern bool current_list;


void generate(string &s, string &regex) {
    
    std::regex r(regex);
    for(std::sregex_iterator i = std::sregex_iterator(s.begin(), s.end(), r);
        i != std::sregex_iterator();
        ++i ) {
        std::smatch m = *i;
        std::cout << m.str() << " at position " << m.position() << " " << m.str().length() << '\n';
        for (int ii = 0; ii < m.size(); ii++) {
            ofLog() << "HEY" << m[ii];
        }
        
    }
}

int parseRegex( string &s, string &search, string &replace , smatch & match, vector<ofxPrecisionTextRegex> & list,  int type) {
    
    int removedCounter = 0;
    
    ofLog() << "---";
    
    
    for( size_t pos = 0; ; pos += replace.length() )
    {
        pos = s.find( search, pos );
        if( pos == string::npos )
            break;
        
        
        ofxPrecisionTextRegex r;
        r.toBeReplaced = search;
        r.originalStart = pos;
        r.start = pos;
        r.size = replace.length();
        r.end = pos + replace.length();
        r.match = replace;
        r.type = type;
        list.push_back(r);
        
        ofLog() << "Add" << pos << search.length() << search.length() - replace.size();
        ofLog() << search;
        
        int off = search.length() - replace.size();
        
        for (auto & rr : list) {
            if (r.start < rr.start) {
                rr.start -= off;
//                rr.end -= off;
//                ofLog() << "Rem" << rr.start;
            }
            if (r.end < rr.end) {
                rr.end -= off;
//                ofLog() << "Rem" << rr.start;
//                ofLog() << rr.toBeReplaced;
            }
        }
        
        s.erase( pos, search.length() );
        
        removedCounter += (search.length() - replace.size());
        string chaos = "";
        for (int i = 0; i < search.length(); i++) chaos += "@";
//        s.insert( pos, chaos );
        s.insert( pos, replace );

    }
    return removedCounter;
}


ofxPrecisionTextStructure parseMarkdown(string & source, bool asHtml) {
    
    
    vector<ofxPrecisionTextRegex> list;
    
    ofxPrecisionTextStructure output;
    output.inSize = source.size();
    output.outSize = 0;
    output.removed = 0;


    /*-- Loop over lines --*/

    std::istringstream f(source);
    string line;
    string outText;
    bool hadNl = ( source.back() == '\n' ); // did it have a newline?

    while (getline(f, line)) {
        line += '\n';
        parseLine(line, output, false, list);
    }

    /*-- Preserve new lines --*/

    if (!hadNl) {
        output.text = output.text.substr( 0 , output.text.size() - 1);
        output.outSize -= 1;
    }
    
    /*-- Parse REGEX candidates to indexes --*/

    for (auto & r : list) {
        if (r.type == PRECISION_BOLD) {
            output.bold.push_back(r.start);
            output.bold.push_back(r.end);
        }
        if (r.type == PRECISION_LINK) {
            
            ofxPrecisionTextHyperlink link;
            link.start = r.start;
            link.end = r.end;
//            link.url = url;
            output.links.push_back(link);
            
        }
//        if (r.type == PRECISION_ITALIC) {
//            output.italic.push_back(r.start);
//            output.italic.push_back(r.end);
//        }
        if (r.type == PRECISION_H1) output.h1.push_back(r.start);
    }
    
    /*-- Check parity of indexes and string lengths --*/
    
    if ((output.text.size() != output.inSize - output.removed) || (output.text.size() != output.outSize)) {
        ofLogError("[ofxPrecisionText]") << "Mismatch in parsed markdown length";
    }

    return output;
}

void parseLine(string& text, ofxPrecisionTextStructure & output, bool asHtml, vector<ofxPrecisionTextRegex> & list) {
    
    
    int removedCounterForLine = 0;

    string stringToReturn = text;
    string toReplace;
    string search;
    smatch match;
    
    if (stringToReturn.empty() && current_list) {
        current_list = false;
        stringToReturn += "</ul>";
    }
    
    if ((stringToReturn.empty() == false) && !(regex_search(stringToReturn, match, bold_regex) || regex_search(stringToReturn, match, italic_regex)
                                               || regex_search(stringToReturn, match, url_regex) || searchHeadersStyle(stringToReturn, match)
                                               || regex_search(stringToReturn, match, img_regex)
                                               || regex_search(stringToReturn, match, list_regex))) {
        if (current_list) {
            current_list = false;
            if (asHtml) toReplace += "</ul>";
            toReplace += stringToReturn;
            stringToReturn = toReplace;
        }
        if (need_paragraph) {
            if (asHtml) toReplace += "<p>";
            toReplace += stringToReturn;
            stringToReturn = toReplace;
            need_paragraph = false;
            current_paragraph = true;
        }
    }
    
    if (stringToReturn.empty() && current_paragraph) {
        if (asHtml) stringToReturn += "</p>";
        need_paragraph = true;
        current_paragraph = false;
    }
    
    
    while (regex_search(stringToReturn, match, bold_regex) || regex_search(stringToReturn, match, italic_regex)
           || regex_search(stringToReturn, match, url_regex) || searchHeadersStyle(stringToReturn, match)
           || regex_search(stringToReturn, match, img_regex)
           || regex_search(stringToReturn, match, list_regex)) {
        if (regex_search(stringToReturn, match, bold_regex)) {
            if (current_list) {
                current_list = false;
                if (asHtml) stringToReturn += "</ul>";
            }
            if (need_paragraph) {
                if (asHtml) toReplace += "<p>";
                need_paragraph = false;
                current_paragraph = true;
            }
            if (asHtml) toReplace += "<b>";
            toReplace += match[1];
            if (asHtml) toReplace += "</b>";
            search += "**";
            search += match[1];
            search += "**";
            removedCounterForLine += parseRegex(stringToReturn, search, toReplace, match, list, PRECISION_BOLD);
        }
        toReplace = "";
        search = "";
        if (regex_search(stringToReturn, match, italic_regex)) {
            if (current_list) {
                current_list = false;
                if (asHtml) toReplace += "</ul>";
            }
            if (need_paragraph) {
                if (asHtml) toReplace += "<p>";
                need_paragraph = false;
                current_paragraph = true;
            }
            if (asHtml) toReplace += "<i>";
            toReplace += match[1];
            if (asHtml) toReplace += "</i>";
            search += "*";
            search += match[1];
            search += "*";
//            removedCounterForLine += parseRegex(stringToReturn, search, toReplace, match, list, PRECISION_ITALIC);
        }
        toReplace = "";
        search = "";
        if (regex_search(stringToReturn, match, list_regex)) {
            if (!current_list) {
                current_list = true;
                if (asHtml) toReplace += "<ul>";
            }
            if (asHtml) toReplace += "<li>";
            toReplace += match[1];
            if (asHtml) toReplace += "</li>";
            search += "*  ";
            search += match[1];
            removedCounterForLine += parseRegex(stringToReturn, search, toReplace, match, list);
        }
        toReplace = "";
        search = "";
        if (regex_search(stringToReturn, match, img_regex)) {
            if (current_list) {
                current_list = false;
                stringToReturn += "</ul>";
            }
            toReplace +="<img src=\"";
            toReplace += match[2];
            toReplace += "\"";
            toReplace += " alt=\"";
            toReplace += match[1];
            toReplace += "\">";
            search += "![";
            search += match[1];
            search += "]";
            search += "(";
            search += match[2];
            search += ")";
            removedCounterForLine += parseRegex(stringToReturn, search, toReplace, match, list);
        }
        toReplace = "";
        search = "";
        if (regex_search(stringToReturn, match, url_regex)) {
            if (current_list) {
                current_list = false;
                stringToReturn += "</ul>";
            }
            if (need_paragraph) {
                if (asHtml) toReplace += "<p>";
                need_paragraph = false;
                current_paragraph = true;
            }
            if (asHtml) toReplace +="<a href=\"";
            if (asHtml) toReplace += match[2];
            string url = match[2];
            if (asHtml) toReplace += "\">";
            toReplace += match[1];
            if (asHtml) toReplace += "</a>";
            search += "[";
            search += match[1];
            search += "]";
            search += "(";
            search += match[2];
            search += ")";
            removedCounterForLine += parseRegex(stringToReturn, search, toReplace, match, list, PRECISION_LINK);
        }
        toReplace = "";
        search = "";
        if (searchHeadersStyle(stringToReturn, match)) {
            if (current_list) {
                current_list = false;
                stringToReturn += "</ul>";
            }
            if (regex_search(stringToReturn, match, h3_regex)) {
                if (asHtml) toReplace += "<h3>";
                toReplace += match[1];
                if (asHtml) toReplace += "</h3>";
                search += "### ";
                search += match[1];
                removedCounterForLine += parseRegex(stringToReturn, search, toReplace, match, list, PRECISION_H3);
            }
            if (regex_search(stringToReturn, match, h2_regex)) {
                if (asHtml) toReplace += "<h2>";
                toReplace += match[1];
                if (asHtml) toReplace += "</h2>";
                search += "## ";
                search += match[1];
                removedCounterForLine += parseRegex(stringToReturn, search, toReplace, match, list, PRECISION_H@);
            }
            if (regex_search(stringToReturn, match, h1_regex)) {
                if (asHtml) toReplace += "<h1>";
                toReplace += match[1];
                if (asHtml) toReplace += "</h1>";
                search += "# ";
                search += match[1];
                removedCounterForLine += parseRegex(stringToReturn, search, toReplace, match, list, PRECISION_H1);
            }
        }
    }

    /*-- Funky stuff to check parity of indexes and string lentghs --*/
    
    
    output.outSize += text.size();
    output.outSize -= removedCounterForLine;
    output.removed += removedCounterForLine;
    output.text += stringToReturn;
    
    return output;
    
}

bool searchHeadersStyle(string& stringToReturn, smatch& match) {
    
    if (regex_search(stringToReturn, match, h1_regex) || regex_search(stringToReturn, match, h2_regex)
        || regex_search(stringToReturn, match, h3_regex))
        return true;
    return false;
    
}
