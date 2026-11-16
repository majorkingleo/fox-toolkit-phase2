/********************************************************************************
*                                                                               *
*                               X M L   T e s t                                 *
*                                                                               *
*********************************************************************************
* Copyright (C) 2016,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include <stdarg.h>
#include "fx.h"
#include "testargs.h"


// Print options
void printusage(const char* prog){
  fxmessage("%s options:\n",prog);
  fxmessage("  --test                   Run self-checks.\n");
  fxmessage("  --load <file>            Load xml file.\n");
  fxmessage("  --save <file>            Save xml file.\n");
  fxmessage("  --oasis <dir>            Run OASIS tests from directory <dir>.\n");
  fxmessage("  --tracelevel <level>     Set trace level.\n");
  fxmessage("  -v, --verbose            Increase verbosity level.\n");
  fxmessage("  -h, --help               Print help.\n");
  }


// Pass or fail
const FXchar *const passed[2]={"fail","pass"};

/*******************************************************************************/

// Save to this
FXXMLFile xmloutput;

// Load from this
FXXMLFile xmlinput;

/*******************************************************************************/

// Just print in the callback

// Start document
FXXML::Error startDocument(){
  fxmessage("startDocument()\n");
  return FXXML::ErrOK;
  }


// Element start
FXXML::Error startElement(const FXString& tag,const FXStringDictionary& atts){
  fxmessage("startElement(%s",tag.text());
  if(!atts.empty()){
    for(FXint i=0; i<atts.no(); ++i){
      if(!atts.empty(i)){ fxmessage(",%s=\"%s\"",atts.key(i).text(),atts.data(i).text()); }
      }
    }
  fxmessage(")\n");
  return FXXML::ErrOK;
  }


// Characters
FXXML::Error characters(const FXString& text){
  fxmessage("characters(%s)\n",text.text());
  return FXXML::ErrOK;
  }


// Comment
FXXML::Error comment(const FXString& text){
  fxmessage("comment(%s)\n",text.text());
  return FXXML::ErrOK;
  }


// Processing instruction
FXXML::Error processing(const FXString& target,const FXString& text){
  fxmessage("processing(%s,%s)\n",target.text(),text.text());
  return FXXML::ErrOK;
  }


// Element end
FXXML::Error endElement(const FXString& tag){
  fxmessage("endElement(%s)\n",tag.text());
  return FXXML::ErrOK;
  }


// Document end
FXXML::Error endDocument(){
  fxmessage("endDocument()\n");
  return FXXML::ErrOK;
  }

/*******************************************************************************/

/*******************************************************************************/

// Failures found
static FXint failures=0;

// Report failure
static void bad(const char* fmt,...){
  va_list ap;
  failures++;
  if(failures<=40){
    va_start(ap,fmt);
    printf("BAD: ");
    vprintf(fmt,ap);
    printf("\n");
    va_end(ap);
    }
  }

// Expect string
static void expectstr(const char* what,const FXString& got,const FXString& want){
  if(got!=want) bad("%s: got \"%s\" expected \"%s\"",what,got.text(),want.text());
  }

// Expect integer
static void expectint(const char* what,FXlong got,FXlong want){
  if(got!=want) bad("%s: got %lld expected %lld",what,(long long)got,(long long)want);
  }

// Expect error code
static void expecterr(const char* what,FXXML::Error got,FXXML::Error want){
  if(got!=want) bad("%s: got \"%s\" expected \"%s\"",what,FXXML::getError(got),FXXML::getError(want));
  }

// Remove CR in CRLF pairs, as the writer produces CRLF on Windows
static FXString unixeol(const FXString& s){
  FXString r;
  for(FXint i=0; i<s.length(); ++i){
    if(s[i]=='\r' && s[i+1]=='\n') continue;
    r.append(s[i]);
    }
  return r;
  }

// Header written by the writer
static const FXchar boiler[]="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

/*******************************************************************************/

// Record all events as one string
class Rec {
public:
  FXString log;
  FXString text;
public:
  void flush(){
    if(!text.empty()){
      log+="T[";
      log+=text;
      log+="]";
      text.clear();
      }
    }
  void attach(FXXML& xml){
    xml.startDocumentCB.connect<Rec,&Rec::startDocument>(this);
    xml.startElementCB.connect<Rec,&Rec::startElement>(this);
    xml.charactersCB.connect<Rec,&Rec::characters>(this);
    xml.commentCB.connect<Rec,&Rec::comment>(this);
    xml.processingCB.connect<Rec,&Rec::processing>(this);
    xml.endElementCB.connect<Rec,&Rec::endElement>(this);
    xml.endDocumentCB.connect<Rec,&Rec::endDocument>(this);
    }
  FXXML::Error startDocument(){
    flush();
    log+="<doc>";
    return FXXML::ErrOK;
    }
  FXXML::Error startElement(const FXString& tag,const FXStringDictionary& atts){
    FXString keys[16];
    FXString vals[16];
    FXint n=0;
    flush();
    for(FXint i=0; i<atts.no(); ++i){
      if(!atts.empty(i) && n<16){
        keys[n]=atts.key(i);
        vals[n]=atts.data(i);
        n++;
        }
      }
    for(FXint i=0; i<n; ++i){
      for(FXint j=i+1; j<n; ++j){
        if(strcmp(keys[j].text(),keys[i].text())<0){
          FXString k=keys[i]; keys[i]=keys[j]; keys[j]=k;
          FXString v=vals[i]; vals[i]=vals[j]; vals[j]=v;
          }
        }
      }
    log+="<";
    log+=tag;
    for(FXint i=0; i<n; ++i){
      log+=" ";
      log+=keys[i];
      log+="=";
      log+=vals[i];
      }
    log+=">";
    return FXXML::ErrOK;
    }
  FXXML::Error characters(const FXString& str){
    text+=str;
    return FXXML::ErrOK;
    }
  FXXML::Error comment(const FXString& str){
    flush();
    log+="C[";
    log+=str;
    log+="]";
    return FXXML::ErrOK;
    }
  FXXML::Error processing(const FXString& target,const FXString& data){
    flush();
    log+="P[";
    log+=target;
    log+="|";
    log+=data;
    log+="]";
    return FXXML::ErrOK;
    }
  FXXML::Error endElement(const FXString& tag){
    flush();
    log+="</";
    log+=tag;
    log+=">";
    return FXXML::ErrOK;
    }
  FXXML::Error endDocument(){
    flush();
    log+="</doc>";
    return FXXML::ErrOK;
    }
  };

// XML writer with access to its buffer
class XMLMem : public FXXML {
public:
  FXString output() const { return FXString(begptr,(FXint)(wptr-begptr)); }
  };

// Parse text in memory
static FXXML::Error parsetext(const FXString& text,Rec& rec,FXint* line=nullptr,FXint* col=nullptr,FXlong* off=nullptr){
  FXString copy(text);
  FXXML xml;
  FXXML::Error err=FXXML::ErrLoad;
  rec.attach(xml);
  if(xml.open(const_cast<FXchar*>(copy.text()),copy.length(),FXXML::Load)){
    err=xml.parse();
    if(line) *line=xml.getLine();
    if(col) *col=xml.getColumn();
    if(off) *off=xml.getOffset();
    xml.close();
    }
  rec.flush();
  return err;
  }

// Parse text and compare events
static void checkparse(const char* what,const FXString& text,const FXString& want){
  Rec rec;
  FXXML::Error err=parsetext(text,rec);
  if(err!=FXXML::ErrOK) bad("%s: parse failed: %s",what,FXXML::getError(err));
  else expectstr(what,rec.log,want);
  }

// Parse text and expect failure
static void checkfail(const char* what,const FXString& text,FXXML::Error want=FXXML::ErrOK){
  Rec rec;
  FXXML::Error err=parsetext(text,rec);
  if(want==FXXML::ErrOK){
    if(err==FXXML::ErrOK) bad("%s: parse succeeded",what);
    }
  else{
    expecterr(what,err,want);
    }
  }

/*******************************************************************************/

// Entity decoding
static void testdecode(){
  FXString out;
  expectint("decode: ok",FXXML::decode(out,"a&lt;b&gt;&amp;&quot;&apos;"),true);
  expectstr("decode: predefined",out,"a<b>&\"'");
  expectint("decode: numeric ok",FXXML::decode(out,"&#65;&#x42;&#x20AC;&#x3b;"),true);
  expectstr("decode: numeric",out,FXString("AB\xE2\x82\xAC")+";");
  expectint("decode: crlf ok",FXXML::decode(out,"a\r\nb\rc\nd"),true);
  expectstr("decode: crlf",out,"a\nb\nc\nd");
  expectint("decode: no flags ok",FXXML::decode(out,"&lt;\r\n",0),true);
  expectstr("decode: no flags",out,"&lt;\r\n");
  expectint("decode: refs only ok",FXXML::decode(out,"&lt;\r\n",FXXML::REFS),true);
  expectstr("decode: refs only",out,"<\r\n");
  expectint("decode: crlf only ok",FXXML::decode(out,"&lt;\r\n",FXXML::CRLF),true);
  expectstr("decode: crlf only",out,"&lt;\n");
  expectint("decode: empty ok",FXXML::decode(out,""),true);
  expectstr("decode: empty",out,"");
  expectint("decode: unknown entity",FXXML::decode(out,"&foo;"),false);
  expectint("decode: no semicolon",FXXML::decode(out,"&amp"),false);
  expectint("decode: lone ampersand",FXXML::decode(out,"a&"),false);
  expectint("decode: no digits",FXXML::decode(out,"&#;"),false);
  expectint("decode: no hex digits",FXXML::decode(out,"&#x;"),false);
  expectint("decode: decimal no semicolon",FXXML::decode(out,"&#65"),false);
  expectint("decode: hex no semicolon",FXXML::decode(out,"&#x41"),false);
  }

// Entity encoding
static void testencode(){
  FXString out;
  FXString back;
  FXString all;
  expectint("encode: ok",FXXML::encode(out,"<a href=\"x\">&'</a>"),true);
  expectstr("encode: specials",out,"&lt;a href=&quot;x&quot;&gt;&amp;&apos;&lt;/a&gt;");
  expectint("encode: semicolon ok",FXXML::encode(out,"a;b"),true);
  expectstr("encode: semicolon",out,"a;b");
  expectint("encode: utf8 ok",FXXML::encode(out,FXString("\xE2\x82\xAC\xE2\x80\x9C")),true);
  expectstr("encode: utf8",out,FXString("\xE2\x82\xAC\xE2\x80\x9C"));
  expectint("encode: cr ok",FXXML::encode(out,"a\rb"),true);
  expectstr("encode: cr",out,"a&#x0D;b");
  expectint("encode: no flags ok",FXXML::encode(out,"<&>",0),true);
  expectstr("encode: no flags",out,"<&>");
  expectint("encode: crlf only ok",FXXML::encode(out,"<\n>",FXXML::CRLF),true);
  expectstr("encode: crlf only",unixeol(out),"<\n>");
  expectint("encode: control ok",FXXML::encode(out,"\x01"),true);
  expectstr("encode: control",out,"&#x01;");
  expectint("encode: empty ok",FXXML::encode(out,""),true);
  expectstr("encode: empty",out,"");
  for(FXint c=1; c<256; ++c) all.append((FXchar)c);
  expectint("encode: all ok",FXXML::encode(out,all),true);
  expectint("encode: all decode ok",FXXML::decode(back,out),true);
  expectstr("encode: all roundtrip",back,all);
  }

// Events of well formed documents
static void testparse(){
  checkparse("empty element","<a/>","<doc><a></a></doc>");
  checkparse("empty element space","<a />","<doc><a></a></doc>");
  checkparse("text","<a>hi</a>","<doc><a>T[hi]</a></doc>");
  checkparse("nested","<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<a x=\"1\" y='two'><b/>t<c k=\"v\">u</c></a>\n","<doc><a x=1 y=two><b></b>T[t]<c k=v>T[u]</c></a></doc>");
  checkparse("attribute spaces","<a  x = \"1\"  y='2' />","<doc><a x=1 y=2></a></doc>");
  checkparse("attribute quotes","<a x='say \"hi\"' y=\"it's\"/>","<doc><a x=say \"hi\" y=it's></a></doc>");
  checkparse("attribute entities","<a t=\"&lt;&amp;&#65;\"/>","<doc><a t=<&A></a></doc>");
  checkparse("attribute empty","<a t=\"\"/>","<doc><a t=></a></doc>");
  checkparse("text entities","<a>&lt;&gt;&amp;&quot;&apos;&#65;&#x42;&#x20AC;</a>",FXString("<doc><a>T[<>&\"'AB\xE2\x82\xAC]</a></doc>"));
  checkparse("cdata","<a>x<![CDATA[<y>&amp;]]>z</a>","<doc><a>T[x<y>&amp;z]</a></doc>");
  checkparse("cdata alone","<a><![CDATA[]]></a>","<doc><a></a></doc>");
  checkparse("comment","<!-- top --><a><!--in-->t<!-- a-b --></a>","C[ top ]<doc><a>C[in]T[t]C[ a-b ]</a></doc>");
  checkparse("processing","<?pi data?><a><?x?>t</a>","P[pi|data]<doc><a>P[x|]T[t]</a></doc>");
  checkparse("whitespace text","<a>\n  <b/>\n</a>","<doc><a>T[\n  ]<b></b>T[\n]</a></doc>");
  checkparse("crlf text","<a>x\r\ny\rz</a>","<doc><a>T[x\ny\nz]</a></doc>");
  checkparse("utf8",FXString("<k\xC3\xA4se>h\xC3\xA4llo \xE2\x82\xAC</k\xC3\xA4se>"),FXString("<doc><k\xC3\xA4se>T[h\xC3\xA4llo \xE2\x82\xAC]</k\xC3\xA4se></doc>"));
  checkparse("name chars","<a-b.c_d:e1/>","<doc><a-b.c_d:e1></a-b.c_d:e1></doc>");
  checkparse("utf8 bom",FXString("\xEF\xBB\xBF<a/>"),"<doc><a></a></doc>");
  checkparse("doctype system","<!DOCTYPE a SYSTEM \"a.dtd\"><a/>","<doc><a></a></doc>");
  checkparse("doctype public","<!DOCTYPE a PUBLIC \"-//X//Y\" \"a.dtd\"><a/>","<doc><a></a></doc>");
  checkparse("doctype subset","<!DOCTYPE a [ <!ELEMENT a ANY> ]><a/>","<doc><a></a></doc>");
  checkparse("deep","<a><b><c><d>x</d></c></b></a>","<doc><a><b><c><d>T[x]</d></c></b></a></doc>");
  }

// Malformed documents
static void testerrors(){
  checkfail("blank","   \n",FXXML::ErrEmpty);
  checkfail("text only","x",FXXML::ErrToken);
  checkfail("mismatch","<a></b>",FXXML::ErrNoMatch);
  checkfail("mismatch nested","<a><b></a></b>",FXXML::ErrNoMatch);
  checkfail("unclosed","<a>",FXXML::ErrEof);
  checkfail("unclosed nested","<a><b></b>",FXXML::ErrEof);
  checkfail("unquoted attribute","<a b=c/>",FXXML::ErrString);
  checkfail("attribute no value","<a b/>",FXXML::ErrEquals);
  checkfail("attribute unterminated","<a b=\"1>",FXXML::ErrEof);
  checkfail("bad name","<1/>",FXXML::ErrName);
  checkfail("bad empty tag","<a/");
  checkfail("bad end tag","<a></a x>");
  checkfail("unknown entity","<a>&foo;</a>");
  checkfail("unterminated entity","<a>&amp</a>");
  checkfail("unknown entity attribute","<a x=\"&foo;\"/>");
  checkfail("unclosed comment","<a><!-- x</a>",FXXML::ErrEof);
  checkfail("unclosed cdata","<a><![CDATA[ x</a>",FXXML::ErrEof);
  checkfail("unclosed processing","<a><?pi x</a>",FXXML::ErrEof);
  checkfail("bad version","<?xml version=1.0?><a/>",FXXML::ErrString);
  checkfail("bad declaration","<?xml foo?><a/>",FXXML::ErrToken);
  }

// Line, column and offset
static void testposition(){
  Rec rec;
  FXint line=0;
  FXint col=0;
  FXlong off=0;
  expecterr("position: ok",parsetext("<a/>",rec,&line,&col,&off),FXXML::ErrOK);
  expectint("position: line",line,1);
  expectint("position: column",col,4);
  expectint("position: offset",off,4);
  rec=Rec();
  expecterr("position text: ok",parsetext("<a>x</a>",rec,&line,&col,&off),FXXML::ErrOK);
  expectint("position text: offset",off,8);
  rec=Rec();
  expecterr("position newlines: ok",parsetext("<a>\n\n</a>",rec,&line,&col,&off),FXXML::ErrOK);
  expectint("position newlines: line",line,3);
  expectint("position newlines: offset",off,9);
  rec=Rec();
  expecterr("position crlf: ok",parsetext("<a>\r\n\r\n</a>",rec,&line,&col,&off),FXXML::ErrOK);
  expectint("position crlf: line",line,3);
  expectint("position crlf: offset",off,11);
  rec=Rec();
  expecterr("position error",parsetext("<a>\n<b>\n</a>",rec,&line,&col,&off),FXXML::ErrNoMatch);
  expectint("position error: line",line,3);
  rec=Rec();
  expecterr("position bom: ok",parsetext(FXString("\xEF\xBB\xBF<a/>"),rec,&line,&col,&off),FXXML::ErrOK);
  expectint("position bom: offset",off,7);
  }

// Writer output
static void testwriter(){
  FXchar buffer[4096];
  XMLMem xml;
  FXStringDictionary atts;
  if(xml.open(buffer,sizeof(buffer),FXXML::Save)){
    atts["y"]="a\"b<&'>";
    expecterr("writer: document",xml.startDocument(),FXXML::ErrOK);
    expecterr("writer: element",xml.startElement("a",atts),FXXML::ErrOK);
    expecterr("writer: text",xml.characters("x<y&z>"),FXXML::ErrOK);
    expecterr("writer: comment",xml.comment("c"),FXXML::ErrOK);
    expecterr("writer: processing",xml.processing("t","d"),FXXML::ErrOK);
    expecterr("writer: processing empty",xml.processing("u",""),FXXML::ErrOK);
    expecterr("writer: child",xml.startElement("b"),FXXML::ErrOK);
    expecterr("writer: child end",xml.endElement("b"),FXXML::ErrOK);
    expecterr("writer: end",xml.endElement("a"),FXXML::ErrOK);
    expecterr("writer: document end",xml.endDocument(),FXXML::ErrOK);
    expectstr("writer: output",unixeol(xml.output()),FXString(boiler)+"<a y=\"a&quot;b&lt;&amp;&apos;&gt;\">x&lt;y&amp;z&gt;<!--c--><?t d?><?u?><b></b></a>\n");
    expectint("writer: offset",xml.getOffset(),(FXlong)xml.output().length());
    xml.close();
    }
  else{
    bad("writer: open failed");
    }

  // Too small a buffer cannot be written
  FXchar tiny[16];
  XMLMem small;
  if(small.open(tiny,sizeof(tiny),FXXML::Save)){
    expecterr("writer: tiny buffer",small.startDocument(),FXXML::ErrSave);
    small.close();
    }

  // Writing while loading is refused
  FXXML reader;
  expecterr("writer: wrong direction",reader.startDocument(),FXXML::ErrSave);
  }

// Parse and write back
static void testroundtrip(){
  const FXString src="<a x=\"1&amp;2\"><b>t&lt;u</b><c/><!--k--><?pi data?><d>\xC3\xA4 &quot;q&quot;</d></a>";
  const FXString want=FXString(boiler)+"<a x=\"1&amp;2\"><b>t&lt;u</b><c></c><!--k--><?pi data?><d>\xC3\xA4 &quot;q&quot;</d></a>\n";
  FXchar buffer[4096];
  FXString copy(src);
  XMLMem out;
  FXXML in;
  if(out.open(buffer,sizeof(buffer),FXXML::Save) && in.open(const_cast<FXchar*>(copy.text()),copy.length(),FXXML::Load)){
    in.startDocumentCB.connect<FXXML,&FXXML::startDocument>(&out);
    in.startElementCB.connect<FXXML,&FXXML::startElement>(&out);
    in.charactersCB.connect<FXXML,&FXXML::characters>(&out);
    in.commentCB.connect<FXXML,&FXXML::comment>(&out);
    in.processingCB.connect<FXXML,&FXXML::processing>(&out);
    in.endElementCB.connect<FXXML,&FXXML::endElement>(&out);
    in.endDocumentCB.connect<FXXML,&FXXML::endDocument>(&out);
    expecterr("roundtrip: parse",in.parse(),FXXML::ErrOK);
    expectstr("roundtrip: output",unixeol(out.output()),want);
    in.close();
    out.close();
    }
  else{
    bad("roundtrip: open failed");
    }

  // Text with every kind of character survives write and parse
  FXString text;
  FXString attr;
  for(FXint c=1; c<128; ++c) text.append((FXchar)c);
  attr=text;
  text+=FXString("\xC3\xA4\xE2\x82\xAC");
  FXXML::Error err;
  if(out.open(buffer,sizeof(buffer),FXXML::Save)){
    FXStringDictionary atts;
    atts["k"]=attr;
    out.startDocument();
    out.startElement("a",atts);
    out.characters(text);
    out.endElement("a");
    out.endDocument();
    FXString xml=out.output();
    out.close();
    Rec rec;
    err=parsetext(xml,rec);
    expecterr("roundtrip chars: parse",err,FXXML::ErrOK);
    expectstr("roundtrip chars: events",rec.log,"<doc><a k="+attr+">T["+text+"]</a></doc>");
    }
  }

// Parse file of given content through a small buffer
static void checkfile(const char* what,const FXString& content,const FXString& want){
  const FXString name="xml_selftest.tmp.xml";
  FXFile file;
  if(file.open(name,FXIO::Writing,FXIO::AllReadWrite)){
    file.writeBlock(content.text(),content.length());
    file.close();
    FXXMLFile in;
    if(in.open(name,FXXML::Load,512)){
      Rec rec;
      rec.attach(in);
      FXXML::Error err=in.parse();
      rec.flush();
      in.close();
      if(err!=FXXML::ErrOK) bad("%s: parse failed: %s",what,FXXML::getError(err));
      else if(rec.log!=want) bad("%s: events differ, got %d bytes expected %d bytes",what,rec.log.length(),want.length());
      }
    else{
      bad("%s: open for reading failed",what);
      }
    FXFile::remove(name);
    }
  else{
    bad("%s: open for writing failed",what);
    }
  }

// Large documents through a small file buffer
static void testfile(){
  const FXString name="xml_selftest.tmp.xml";
  FXString ws,mixed,mixedraw,val,valraw,comment,cdata,words;
  FXXML::Error err;
  Rec rec;
  for(FXint i=0; i<400; ++i) ws+="   \n  \t";
  for(FXint i=0; i<300; ++i){
    mixedraw+=FXString("h\xC3\xA4llo &amp; \xE2\x82\xAC &lt;x&gt; ");
    mixed+=FXString("h\xC3\xA4llo & \xE2\x82\xAC <x> ");
    }
  for(FXint i=0; i<200; ++i){
    valraw+="v&amp;l&lt; ";
    val+="v&l< ";
    }
  for(FXint i=0; i<300; ++i){
    comment+="remark - ";
    cdata+="<> & ";
    words+="word ";
    }

  // Each long construct on its own
  checkfile("file text",FXString("<a>")+words+"</a>","<doc><a>T["+words+"]</a></doc>");
  checkfile("file text entities",FXString("<a>")+mixedraw+"</a>","<doc><a>T["+mixed+"]</a></doc>");
  checkfile("file text space",FXString("<a>")+ws+"</a>","<doc><a>T["+ws+"]</a></doc>");
  checkfile("file attribute",FXString("<a v=\"")+valraw+"\"/>","<doc><a v="+val+"></a></doc>");
  checkfile("file attribute space",FXString("<a v=\"")+ws+"\"/>","<doc><a v="+ws+"></a></doc>");
  checkfile("file comment",FXString("<a><!--")+comment+"--></a>","<doc><a>C["+comment+"]</a></doc>");
  checkfile("file comment space",FXString("<a><!--")+ws+"--></a>","<doc><a>C["+ws+"]</a></doc>");
  checkfile("file cdata",FXString("<a><![CDATA[")+cdata+"]]></a>","<doc><a>T["+cdata+"]</a></doc>");
  checkfile("file cdata space",FXString("<a><![CDATA[")+ws+"]]></a>","<doc><a>T["+ws+"]</a></doc>");
  checkfile("file processing",FXString("<a><?pi ")+words+"?></a>","<doc><a>P[pi|"+words+"]</a></doc>");
  checkfile("file processing space",FXString("<a><?pi x")+ws+"y?></a>","<doc><a>P[pi|x"+ws+"y]</a></doc>");
  checkfile("file many elements",FXString("<a>")+words+"<b>x</b>"+words+"<c/>"+words+"</a>","<doc><a>T["+words+"]<b>T[x]</b>T["+words+"]<c></c>T["+words+"]</a></doc>");

  // Written and read through a small buffer
  FXXMLFile out;
  if(out.open(name,FXXML::Save,512)){
    FXStringDictionary atts;
    atts["big"]=val;
    out.startDocument();
    out.startElement("root",atts);
    out.comment(comment);
    out.startElement("m");
    out.characters(mixed);
    out.endElement("m");
    out.endElement("root");
    out.endDocument();
    out.close();
    FXXMLFile in;
    if(in.open(name,FXXML::Load,512)){
      rec.attach(in);
      err=in.parse();
      rec.flush();
      in.close();
      expecterr("file roundtrip: parse",err,FXXML::ErrOK);
      expectstr("file roundtrip: events",rec.log,"<doc><root big="+val+">C["+comment+"]<m>T["+mixed+"]</m></root></doc>");
      }
    else{
      bad("file roundtrip: open for reading failed");
      }
    }
  else{
    bad("file roundtrip: open for writing failed");
    }
  FXFile::remove(name);
  }

/*******************************************************************************/

// Run all checks
static int selftest(){
  testdecode();
  testencode();
  testparse();
  testerrors();
  testposition();
  testwriter();
  testroundtrip();
  testfile();
  if(failures){
    printf("FAILED: %d check(s)\n",failures);
    return 1;
    }
  printf("all checks passed\n");
  return 0;
  }

// Test XML I/O
int main(int argc,char *argv[]){
  fxtestargs(argc,argv);
  if(2<=argc && strcmp(argv[1],"--test")==0) return selftest();
  const FXchar* xmltests="xmltests.json";
  const FXchar* loadfile=nullptr;
  const FXchar* savefile=nullptr;
  const FXchar* oasisdir=nullptr;
  FXint verbosity=0;

  // Grab a few arguments
  for(FXint arg=1; arg<argc; ++arg){
    if(strcmp(argv[arg],"-h")==0 || strcmp(argv[arg],"--help")==0){
      printusage(argv[0]);
      exit(0);
      }
    if(strcmp(argv[arg],"-v")==0 || strcmp(argv[arg],"--verbose")==0){
      verbosity++;
      }
    else if(strcmp(argv[arg],"--save")==0){
      if(++arg>=argc){ fxmessage("Missing save filename argument.\n"); exit(1); }
      savefile=argv[arg];
      }
    else if(strcmp(argv[arg],"--load")==0){
      if(++arg>=argc){ fxmessage("Missing load filename argument.\n"); exit(1); }
      loadfile=argv[arg];
      }
    else if(strcmp(argv[arg],"--oasis")==0){
      if(++arg>=argc){ fxmessage("Missing oasis directory argument.\n"); exit(1); }
      oasisdir=argv[arg];
      }
    else if(strcmp(argv[arg],"--tracelevel")==0){
      if(++arg>=argc){ fxmessage("Missing tracelevel number argument.\n"); exit(1); }
      setTraceLevel(strtoul(argv[arg],nullptr,0));
      }
    else{
      fxmessage("Bad argument.\n");
      printusage(argv[0]);
      exit(1);
      }
    }

  // Run tests
  if(oasisdir){
    FXVariant testcases;

    {
    FXJSONFile json;
    if(json.open(xmltests,FXJSON::Load)){
      fxmessage("Loading tests from: %s\n\n",xmltests);
      FXJSON::Error jsonerr=json.load(testcases);
      json.close();
      if(jsonerr!=FXJSON::ErrOK){
        fxmessage("Error: %s:%d:%d: %s\n",xmltests,json.getLine(),json.getColumn(),FXJSON::getError(jsonerr));
        return 1;
        }
      }
    else{
      fxwarning("Unable to open %s.\n",xmltests);
      return 1;
      }
    }

    // Run test cases
    for(FXint i=0; i<testcases.no(); i++){
      FXString pass=testcases[i]["type"];
      FXString desc=testcases[i]["description"];
      FXString file=FXPath::absolute(oasisdir,testcases[i]["filename"]);
      FXXMLFile xml;
      if(xml.open(file,FXXML::Load)){
        FXXML::Error xmlerr=xml.parse();
        FXbool expect=(pass=="pass");
        FXbool actual=(xmlerr==FXXML::ErrOK);
        if(actual!=expect){
          if(2<verbosity){ fxmessage("\nDescription: %s\n",desc.text()); }
          fxmessage("Fail: %s",file.text());
          if(0<verbosity && !actual){
            fxmessage(":%d:%d: %s",xml.getLine(),xml.getColumn(),FXXML::getError(xmlerr));
            }
          if(1<verbosity){ fxmessage(" (expected %s)",passed[expect]); }
          fxmessage("\n");
          }
        else{
          if(2<verbosity){ fxmessage("\nDescription: %s\n",desc.text()); }
          fxmessage("Pass: %s",file.text());
          if(0<verbosity && !actual){
            fxmessage(":%d:%d: %s",xml.getLine(),xml.getColumn(),FXXML::getError(xmlerr));
            }
          if(1<verbosity){ fxmessage(" (expected %s)",passed[expect]); }
          fxmessage("\n");
          }
        xml.close();
        }
      else{
        fxwarning("\n\nUnable to open: \"%s\".\n",file.text());
        }
      }
    }

  // Unless output file is specified, we will just print
  // Notice we can connect plain functions as well as member
  // functions of some object
  xmlinput.startDocumentCB.connect<startDocument>();
  xmlinput.startElementCB.connect<startElement>();
  xmlinput.charactersCB.connect<characters>();
  xmlinput.commentCB.connect<comment>();
  xmlinput.processingCB.connect<processing>();
  xmlinput.endElementCB.connect<endElement>();
  xmlinput.endDocumentCB.connect<endDocument>();

  // Open save file
  if(savefile){
    if(!xmloutput.open(savefile,FXXML::Save)){
      fxwarning("Error: unable to open: \"%s\" for writing.\n",savefile);
      return 0;
      }

    // Connect source XML file parser to write back out to destination file
    // Just for kicks, we disconnect comments and processing instructions
    xmlinput.startDocumentCB.connect<FXXML,&FXXML::startDocument>(&xmloutput);
    xmlinput.startElementCB.connect<FXXML,&FXXML::startElement>(&xmloutput);
    xmlinput.charactersCB.connect<FXXML,&FXXML::characters>(&xmloutput);
    xmlinput.commentCB.disconnect();
    xmlinput.processingCB.disconnect();
    xmlinput.endElementCB.connect<FXXML,&FXXML::endElement>(&xmloutput);
    xmlinput.endDocumentCB.connect<FXXML,&FXXML::endDocument>(&xmloutput);
    }

  // Now load some XML file, and invoke the callbacks as we go
  if(loadfile){
    FXXML::Error xmlerr;
    if(!xmlinput.open(loadfile,FXXML::Load)){
      fxwarning("Error: unable to open: \"%s\" for reading.\n",loadfile);
      return 0;
      }
    xmlerr=xmlinput.parse();
    fxmessage("Loaded %lld bytes\n",xmlinput.getOffset());
    if(xmlerr!=FXXML::ErrOK){
      fxmessage("Error: %s:%d:%d: %s\n",loadfile,xmlinput.getLine(),xmlinput.getColumn(),FXXML::getError(xmlerr));
      }
    else{
      fxmessage("OK\n");
      }
    xmlinput.close();
    }

  // Close if it was open
  if(savefile){
    fxmessage("Stored %lld bytes\n",xmloutput.getOffset());
    xmloutput.close();
    fxmessage("OK\n");
    }

  return 0;
  }


