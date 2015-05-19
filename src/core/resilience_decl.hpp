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
        std::queue<int> _freeResilienceNodes;
        std::list<int> _usedResilienceNodes;
        int _resilienceTreeFileDescriptor;
        char * _resilienceTreeFilepath;

        // RELATED TO RESILIENCE RESULTS
        void * _resilienceResults;
        Lock _resilienceResultsLock;
        std::map<int, size_t> _freeResilienceResults;
        int _resilienceResultsFileDescriptor;
        char * _resilienceResultsFilepath;

        // COMMON
        size_t _RESILIENCE_MAX_FILE_SIZE;

        public:
        ResiliencePersistence();
        ~ResiliencePersistence();

        // RELATED TO RESILIENCE TREE
        ResilienceNode * getFreeResilienceNode( ResilienceNode * parent = NULL );
        ResilienceNode * getResilienceNode( int offset );
        void freeResilienceNode( int index );

        // RELATED TO RESILIENCE RESULTS
        void * getResilienceResultsFreeSpace( size_t size );
        void * getResilienceResults( int offset );
        void freeResilienceResultsSpace( int offset, size_t size);
        void restoreResilienceResultsSpace( int offset, size_t size );

    };

    class ResilienceNode {

        bool _inUse;
        bool _computed;
        int _desc;
        int _next;
        int _resultIndex;
        size_t _resultSize;
        size_t _descSize;
        unsigned int _lastDescRestored;

        void addNext( ResilienceNode * rn );

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
        void addDesc( ResilienceNode * rn );
        void removeAllDescs();
    };

}
#endif /* NANOS_RESILIENCE_DECL_H */
