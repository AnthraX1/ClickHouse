#pragma once

#include <boost/noncopyable.hpp>
#include <base/types.h>
#include <functional>
#include <memory>
#include <vector>

#include <Poco/Timestamp.h>

namespace DB
{
class ReadBuffer;
class ReadBufferFromFileBase;
class SeekableReadBuffer;

/// Interface for reading an archive.
class IArchiveReader : public std::enable_shared_from_this<IArchiveReader>, boost::noncopyable
{
public:
    virtual ~IArchiveReader() = default;

    /// Returns true if there is a specified file in the archive.
    virtual bool fileExists(const String & filename) = 0;

    struct FileInfo
    {
        UInt64 uncompressed_size;
        UInt64 compressed_size;
        Poco::Timestamp last_modified;
        bool is_encrypted;
    };

    /// Returns the information about a file stored in the archive.
    virtual FileInfo getFileInfo(const String & filename) = 0;

    class FileEnumerator
    {
    public:
        virtual ~FileEnumerator() = default;
        virtual const String & getFileName() const = 0;
        virtual const FileInfo & getFileInfo() const = 0;
        virtual bool nextFile() = 0;
    };

    virtual const std::string & getPath() const = 0;

    /// Starts enumerating files in the archive.
    virtual std::unique_ptr<FileEnumerator> firstFile() = 0;

    using NameFilter = std::function<bool(const std::string &)>;

    /// Starts reading a file from the archive. The function returns a read buffer,
    /// you can read that buffer to extract uncompressed data from the archive.
    /// Several read buffers can be used at the same time in parallel.
    virtual std::unique_ptr<ReadBufferFromFileBase> readFile(const String & filename, bool throw_on_not_found) = 0;
    virtual std::unique_ptr<ReadBufferFromFileBase> readFile(NameFilter filter, bool throw_on_not_found) = 0;

    /// It's possible to convert a file enumerator to a read buffer and vice versa.
    virtual std::unique_ptr<ReadBufferFromFileBase> readFile(std::unique_ptr<FileEnumerator> enumerator) = 0;
    virtual std::unique_ptr<FileEnumerator> nextFile(std::unique_ptr<ReadBuffer> read_buffer) = 0;
    virtual std::unique_ptr<FileEnumerator> currentFile(std::unique_ptr<ReadBuffer> read_buffer) = 0;

    virtual std::vector<std::string> getAllFiles() = 0;
    virtual std::vector<std::string> getAllFiles(NameFilter filter) = 0;

    /// Streams all files in the archive sequentially, calling the callback for each file.
    /// The callback receives the filename and a ReadBuffer containing the file's data.
    /// Returns false to stop iteration, or true to continue to the next file.
    /// Returns true if this archive format supports sequential streaming (e.g., tar),
    /// or false if this optimization is not applicable (e.g., zip has efficient random access).
    /// This method is useful for tar archives where random access requires O(N) scan for each file,
    /// leading to O(N²) total complexity. Sequential streaming achieves O(N) complexity.
    virtual bool streamAllFiles(std::function<bool(const String & filename, ReadBuffer & read_buffer)> /* callback */)
    {
        return false; // Default: not supported
    }

    /// Sets password used to decrypt files in the archive.
    virtual void setPassword(const String & /* password */) {}

    using ReadArchiveFunction = std::function<std::unique_ptr<SeekableReadBuffer>()>;
};

}
