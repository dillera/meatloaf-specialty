// CS:/ - a scheme for handling CommodoreServer Internet Protocol
// see: https://www.commodoreserver.com/BlogEntryView.asp?EID=9D133160E7C344A398EC1F45AEF4BF32
//

#ifndef MEATFILESYSTEM_SCHEME_CS
#define MEATFILESYSTEM_SCHEME_CS

#include "../../include/global_defines.h"
#include "../../include/make_unique.h"
#include "meat_io.h"
#include "tcp.h"

#include "fnSystem.h"
#include "fnTcpClient.h"

#include "utils.h"
#include "string_utils.h"

#include <streambuf>
#include <istream>

/********************************************************
 * Telnet buffer
 ********************************************************/

class csstreambuf : public std::streambuf {
    char* gbuf;
    char* pbuf;

protected:
    MeatSocket m_wifi;

public:
    csstreambuf() {}

    ~csstreambuf() {
        close();
    }      

    bool is_open() {
        return (m_wifi.isOpen());
    }

    bool open() {
        if(m_wifi.isOpen())
            return true;

        int rc = m_wifi.open("commodoreserver.com", 1541);
        Serial.printf("csstreambuf: connect to cserver returned: %d\n", rc);

        if(rc == 1) {
            if(gbuf == nullptr)
                gbuf = new char[512];
            if(pbuf == nullptr)
                pbuf = new char[512];
        }

        setp(pbuf, pbuf+512);

        return rc == 1;
    }

    void close() {
        Serial.printf("csstreambuf: closing\n");
        if(m_wifi.isOpen()) {
            m_wifi.close();
        }
        if(gbuf != nullptr)
            delete[] gbuf;
        if(pbuf != nullptr)
            delete[] pbuf;
    }

    int underflow() override {
        Debug_printv("In underflow");
        if (!m_wifi.isOpen()) {
            Debug_printv("In connection closed");
            close();
            return std::char_traits<char>::eof();
        }
        else if (this->gptr() == this->egptr()) {
            int readCount = 0;
            int attempts = 5;
            int wait = 500;
            
            readCount = m_wifi.read((uint8_t*)gbuf, 512);

            while( readCount <= 0 && (attempts--)>0 && m_wifi.isOpen()) {
                Debug_printv("got rc: %d, retrying", readCount);
                fnSystem.delay(wait);
                wait+=100;
                readCount = m_wifi.read((uint8_t*)gbuf, 512);
            } 
            Debug_printv("read success: %d", readCount);
            this->setg(gbuf, gbuf, gbuf + readCount);
        }
        else {
            Debug_printv("else: %d - %d, (%d)", this->gptr(), this->egptr(), this->gbuf);
        }

        return this->gptr() == this->egptr()
            ? std::char_traits<char>::eof()
            : std::char_traits<char>::to_int_type(*this->gptr());
    };


    int overflow(int ch  = traits_type::eof()) override
    {
        //Debug_printv("in overflow");

        if (!m_wifi.isOpen()) {
            close();
            return EOF;
        }

        char* end = pptr();
        if ( ch != EOF ) {
            *end ++ = ch;
        }

        uint8_t* pBase = (uint8_t*)pbase();

        if ( m_wifi.write( pBase, end - pbase() ) == 0 ) {
            ch = EOF;
        } else if ( ch == EOF ) {
            ch = 0;
        }
        setp(pbuf, pbuf+512);
        
        return ch;
    };

    int sync() { 

        if (!m_wifi.isOpen()) {
            close();
            return 0;
        }
        if(pptr() == pbase()) {
            return 0;
        }
        else {
            //Debug_printv("in sync, written %d", pptr()-pbase());
            uint8_t* buffer = (uint8_t*)pbase();
            auto result = m_wifi.write(buffer, pptr()-pbase()); 
            setp(pbuf, pbuf+512);
            return (result != 0) ? 0 : -1;  
        }  
    };

    friend class CServerSessionMgr;
};

/********************************************************
 * Session manager
 ********************************************************/

class CServerSessionMgr : public std::iostream {
    std::string m_user;
    std::string m_pass;
    csstreambuf buf;

protected:
    std::string currentDir;

    bool establishSession();

    bool sendCommand(std::string);
    
    bool traversePath(MFile* path);

    bool isOK();

    std::string readLn();

public:
    CServerSessionMgr(std::string user = "", std::string pass = "") : std::iostream(&buf), m_user(user), m_pass(pass)
    {};

    ~CServerSessionMgr() {
        sendCommand("quit");
    };

    // read/write are used only by MStream
    size_t receive(uint8_t* buffer, size_t size) {
        if(buf.is_open())
            return buf.m_wifi.read(buffer, size);
        else
            return 0;
    }

    // read/write are used only by MStream
    size_t send(const uint8_t* buffer, size_t size) {
        if(buf.is_open())
            return buf.m_wifi.write(buffer, size);
        else
            return 0;
    }

    bool is_open() {
        return buf.is_open();
    }

    friend class CServerFile;
    friend class CServerIStream;
    friend class CServerOStream;
};

/********************************************************
 * File implementations
 ********************************************************/
class CServerFile: public MFile {

public:
    CServerFile(std::string path, size_t size = 0): MFile(path), m_size(size) 
    {
        media_blocks_free = 65535;
        media_block_size = 1; // blocks are already calculated
        parseUrl(path);
        // Debug_printv("path[%s] size[%d]", path.c_str(), size);
    };

    MStream* createIStream(std::shared_ptr<MStream> src) { return src.get(); };
    MStream* meatStream() override ; // has to return OPENED stream

    std::string petsciiName() override {
        return name;
    }

    //MFile* cd(std::string newDir);
    bool isDirectory() override;
    bool rewindDirectory() override;
    MFile* getNextFileInDir() override;
    bool mkDir() override ;

    bool exists() override;
    bool remove() override;
    bool rename(std::string dest) { return false; };
    time_t getLastWrite() override { return 0; };
    time_t getCreationTime() override { return 0; };
    size_t size() override;     

    bool isDir = true;
    bool dirIsOpen = false;

private:
    bool dirIsImage = false;
    size_t m_size;
};

/********************************************************
 * Streams
 ********************************************************/

//
class CServerIStream: public MStream {

public:
    CServerIStream(std::string path) {
        url = path;
    }
    ~CServerIStream() {
        close();
    }
    // MStream methods
    bool open() override;
    void close() override;

    size_t available() override;
    size_t size() override;
    size_t position() override;

    virtual bool seek(size_t pos) {
        return false;
    };

    size_t read(uint8_t* buf, size_t size) override;
    size_t write(const uint8_t *buf, size_t size) override;
    bool isOpen() override;

protected:
    std::string url;
    bool m_isOpen;
    size_t m_length;
    size_t m_bytesAvailable = 0;
    size_t m_position = 0;
};



/********************************************************
 * FS
 ********************************************************/

class CServerFileSystem: public MFileSystem 
{
    bool handles(std::string name) {
        return name == "cs:";
    }
    
public:
    CServerFileSystem(): MFileSystem("c=server") {};
    static CServerSessionMgr session;
    MFile* getFile(std::string path) override {
        return new CServerFile(path);
    }

};




#endif /* MEATFILESYSTEM_SCHEME_CS */
