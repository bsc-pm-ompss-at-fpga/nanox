#ifndef NANOS_RESILIENCE_DECL_H
#define NANOS_RESILIENCE_DECL_H

#include "copydata_decl.hpp"
#include "resilience_fwd.hpp"
#include <queue>
#include <list>
#include <map>
#include "atomic_decl.hpp"

namespace nanos {

    class ResiliencePersistence {

        // RELATED TO RESILIENCE TREE
        ResilienceNode * _resilienceTree;
        Lock _resilienceTreeLock;
        std::queue<unsigned int> _freeResilienceNodes;
        std::list<unsigned int> _usedResilienceNodes;
        int _resilienceTreeFileDescriptor;
        char * _resilienceTreeFilepath;
        size_t _RESILIENCE_TREE_MAX_FILE_SIZE;

        // RELATED TO RESILIENCE RESULTS
        void * _resilienceResults;
        Lock _resilienceResultsLock;
        std::map<unsigned int, size_t> _freeResilienceResults;
        int _resilienceResultsFileDescriptor;
        char * _resilienceResultsFilepath;
        size_t _RESILIENCE_RESULTS_MAX_FILE_SIZE;

        void removeAllDescs( ResilienceNode * rn );
        void printResilienceInfo();

        public:
        ResiliencePersistence( int rank, size_t resilienceTreeFileSize, size_t resilienceResultsFileSize );
        ~ResiliencePersistence();

        // RELATED TO RESILIENCE TREE
        ResilienceNode * getFreeResilienceNode( ResilienceNode * parent = NULL );
        ResilienceNode * getResilienceNode( unsigned int offset );
        void freeResilienceNode( unsigned int offset );

        // RELATED TO RESILIENCE RESULTS
        void * getResilienceResultsFreeSpace( size_t size );
        void * getResilienceResults( unsigned int offset );
        void freeResilienceResultsSpace( unsigned int offset, size_t size);
        void restoreResilienceResultsSpace( unsigned int offset, size_t size );

    };

    class ResilienceNode {
        friend class ResiliencePersistence;

        bool _inUse;
        bool _computed;
        int _desc;
        int _next;
        int _resultIndex;
        size_t _resultSize;
        size_t _descSize;
        unsigned int _lastDescRestored;

        bool addNext( int next );

        public:

        //_inUse
        bool isInUse() const;
        void setInUse( bool flag );

        //_computed
        bool isComputed() const;

        //_resultIndex
        int getResultIndex() const;

        //_resultSize
        size_t getResultSize() const;

        //_descSize
        size_t getNumDescendants();

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
