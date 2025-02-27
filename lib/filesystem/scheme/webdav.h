// HTTP:// - Hypertext Transfer Protocol

#ifndef MEATFILE_DEFINES_FSHTTP_H
#define MEATFILE_DEFINES_FSHTTP_H

#include "http.h"

#include "meat_io.h"
#include "../../include/global_defines.h"


/********************************************************
 * File implementations
 ********************************************************/


class WebDAVFile: public MFile {

public:
    WebDAVFile(std::string path): MFile(path) {};

    bool isDirectory() override;
    MStream* meatStream() override ; // has to return OPENED stream
    time_t getLastWrite() override ;
    time_t getCreationTime() override ;
    bool rewindDirectory() override { return false; };
    MFile* getNextFileInDir() override { return nullptr; };
    bool mkDir() override { return false; };
    bool exists() override ;
    size_t size() override ;
    bool remove() override { return false; };
    bool rename(std::string dest) { return false; };
    MStream* createIStream(std::shared_ptr<MStream> src);
    //void addHeader(const String& name, const String& value, bool first = false, bool replace = true);
};


/********************************************************
 * Streams
 ********************************************************/

class WebDAVIOStream: public MStream {
public:
    WebDAVIOStream(std::string& path) {
        url = path;
    }
    ~WebDAVIOStream() {
        close();
    }

    void close() override;
    bool open() override;

    // MStream methods
    size_t position() override;
    size_t available() override;
    size_t read(uint8_t* buf, size_t size) override;
    size_t write(const uint8_t *buf, size_t size) override;
    bool isOpen();

protected:
    std::string url;
    bool m_isOpen;
    size_t m_length;
    size_t m_bytesAvailable = 0;
    size_t m_position = 0;
       
//    WiFiClient m_file;
	MeatHttpClient m_http;
};


class WebDAVIStream: public MStream {

public:
    WebDAVIStream(std::string path) {
        m_http.set_header("user-agent", USER_AGENT);
        //m_http.setUserAgent(USER_AGENT);
        //m_http.setTimeout(10000);
        //m_http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        //m_http.setRedirectLimit(10);
        url = path;
    }
    ~WebDAVIStream() {
        close();
    }

    // MStream methods
    size_t size() override;
    size_t available() override;     
    size_t position() override;

    virtual bool seek(size_t pos);

    void close() override;
    bool open() override;

    // MStream methods
    size_t read(uint8_t* buf, size_t size) override;
    bool isOpen();

protected:
    std::string url;
    bool m_isOpen;
    size_t m_bytesAvailable = 0;
    size_t m_length = 0;
    size_t m_position = 0;
    bool isFriendlySkipper = false;

//    WiFiClient m_file;
	MeatHttpClient m_http;
};


class WebDAVOStream: public MStream {

public:
    // MStream methods
    WebDAVOStream(std::string path) {
        m_http.set_header("user-agent", USER_AGENT);
        //m_http.setTimeout(10000);
        //m_http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        //m_http.setRedirectLimit(10);
        //m_http.setReuse(true);

        url = path;
    }
    ~WebDAVOStream() {
        close();
    }

    // MStream methods
    size_t size() override;
    size_t available() override;     
    size_t position() override;

    virtual bool seek(size_t pos);

    void close() override;
    bool open() override;


    // MStream methods
    size_t write(const uint8_t *buf, size_t size) override;
    bool isOpen();

protected:
    std::string url;
    bool m_isOpen;
    size_t m_bytesAvailable = 0;
    size_t m_length = 0;
    size_t m_position = 0;
    bool isFriendlySkipper = false;
    
//    WiFiClient m_file;
    //WiFiClient m_client;
	MeatHttpClient m_http;
};


/********************************************************
 * FS
 ********************************************************/

class WebDAVFileSystem: public MFileSystem 
{
    MFile* getFile(std::string path) override {
        return new WebDAVFile(path);
    }

    bool handles(std::string name) {
        std::string pattern = "http:";
        return mstr::equals(name, pattern, false);
    }
public:
    WebDAVFileSystem(): MFileSystem("http") {};
};


#endif