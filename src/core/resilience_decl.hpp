#ifndef NANOS_RESILIENCE_DECL_H
#define NANOS_RESILIENCE_DECL_H

#include "copydata_decl.hpp"
#include "resilience_fwd.hpp"
#include <queue>
#include <list>
#include <map>
#include "atomic_decl.hpp"

namespace nanos {

    struct FreeInfo
    {
        size_t size;
        unsigned long next_free;
    };

    union ResultsNode
    {
        FreeInfo free_info;
        char * data;
    };

    class ResiliencePersistence {

        bool _restoreTree;

        // RELATED TO RESILIENCE TREE
        ResilienceNode * _resilienceTree;
        int _firstFreeResilienceNode;
        int _firstUsedResilienceNode;
        Lock _resilienceTreeLock;
        int _resilienceTreeFileDescriptor;
        char * _resilienceTreeFilepath;
        size_t _RESILIENCE_TREE_MAX_FILE_SIZE;

        // RELATED TO RESILIENCE RESULTS
        void * _resilienceResults;
        unsigned long _freeResilienceResults;
        Lock _resilienceResultsLock;
        int _resilienceResultsFileDescriptor;
        char * _resilienceResultsFilepath;
        size_t _RESILIENCE_RESULTS_MAX_FILE_SIZE;

        void removeAllDescs( ResilienceNode * rn );
        //void printResilienceInfo();

        public:
        ResiliencePersistence( int rank, size_t resilienceTreeFileSize, size_t resilienceResultsFileSize );
        ~ResiliencePersistence();

        // RELATED TO RESILIENCE TREE
        ResilienceNode * getFreeResilienceNode( ResilienceNode * parent = NULL );
        ResilienceNode * getResilienceNode( unsigned int offset );
        void freeResilienceNode( unsigned int offset );

        // RELATED TO RESILIENCE RESULTS
        unsigned long getResilienceResultsFreeSpace( size_t size, unsigned char &extraSize );
        void * getResilienceResults( unsigned long offset );
        void freeResilienceResultsSpace( unsigned long offset, size_t size);

        void printResilienceInfo();
    };

    class ResilienceNode {
        friend class ResiliencePersistence;

        int _desc;
        int _sibling;
        int _next;
        int _prev;
        unsigned int _descSize;
        unsigned int _lastDescRestored;
        unsigned long _resultIndex;
        size_t _resultSize;
        unsigned char _extraSize;
        bool _inUse;
        bool _computed;

        bool addSibling( int sibling );
        size_t getResultSizeToFree() const;

        public:

        //_inUse
        bool isInUse() const;
        void setInUse( bool flag );

        //_computed
        bool isComputed() const;

        //_resultIndex
        unsigned long getResultIndex() const;

        //_resultSize
        size_t getResultSize() const;

        //_descSize
        unsigned int getNumDescendants();

        // METHODS RELATED TO RESULT
        void storeResult( CopyData * copies, size_t numCopies, int task_id );
        void loadResult( CopyData * copies, size_t numCopies, int task_id );

        // METHODS RELATED TO RESTORE
        void restartLastDescRestored();
        ResilienceNode* getNextDescToRestore();

        // METHODS RELATED TO TREE
        bool addDesc( int desc );
        void removeAllDescs();
    };

}
#endif /* NANOS_RESILIENCE_DECL_H */
