#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "resilience_decl.hpp"
#include "resilience.hpp"
#include "copydata.hpp"
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace nanos {

    /********** RESILIENCE PERSISTENCE **********/

    ResiliencePersistence::ResiliencePersistence( int rank ) :
      _resilienceTree( NULL )
      , _resilienceTreeFileDescriptor( -1 )
      , _resilienceTreeFilepath( NULL )
      , _resilienceResults( NULL )
      , _resilienceResultsFileDescriptor( -1 )
      , _resilienceResultsFilepath( NULL )
      , _RESILIENCE_MAX_FILE_SIZE( 16*1024*1024*sizeof( ResilienceNode ) )
    {
        /* Get path of executable file. With this path, the files for store the persistent resilience tree and the persistent resilience results are created 
         * with the same name of the executable, in the same path, terminating with ".tree" and ".results" respectively.
         */
        bool restoreTree = true;
        char link[32];
        sprintf( link, "/proc/%d/exe", getpid() );
        char path[256];
        int pos = readlink ( link, path, sizeof( path ) );
        if( pos <= 0 )
            fatal0( "Resilience: Error getting path of executable file" );
        _resilienceTreeFilepath = NEW char[pos+8+sizeof(".tree")];
        _resilienceResultsFilepath = NEW char[pos+8+sizeof(".results")];
        memset( _resilienceTreeFilepath, 0, pos+sizeof(".tree") );
        memset( _resilienceResultsFilepath, 0, pos+sizeof(".results") );
        strncpy( _resilienceTreeFilepath, path, pos );
        strncpy( _resilienceResultsFilepath, _resilienceTreeFilepath, pos );

        // MPI
        if( rank != -1 ) {
            sprintf( _resilienceTreeFilepath, "%s.%d", _resilienceTreeFilepath, rank );
            sprintf( _resilienceResultsFilepath, "%s.%d", _resilienceResultsFilepath, rank );
        }
        // /MPI
        strcat( _resilienceTreeFilepath, ".tree" );
        strcat( _resilienceResultsFilepath, ".results" );

        //Resilience tree mmapped file must be created before any WD because in WD constructor we will use the memory mapped.
        if( _resilienceTree == NULL ) {
            // Check if we have a file with the resilience tree from past executions.
            _resilienceTreeFileDescriptor = open( _resilienceTreeFilepath, O_RDWR );

            //If there isn't a file from past executions.
            if( _resilienceTreeFileDescriptor == -1 ) {
                restoreTree = false;
                _resilienceTreeFileDescriptor = open( _resilienceTreeFilepath, O_RDWR | O_CREAT, (mode_t) 0600 );
                if( _resilienceTreeFileDescriptor == -1 )
                    fatal0( "Resilience: Error creating persistentTree." );

                //Stretch file.
                int res = lseek( _resilienceTreeFileDescriptor, _RESILIENCE_MAX_FILE_SIZE - 1, SEEK_SET );
                if( res == -1 ) {
                    close( _resilienceTreeFileDescriptor );
                    fatal0( "Resilience: Error calling lseek." );
                }

                /* Something needs to be written at the end of the file to force the file have actually the new size.
                 * Just writing an empty string at the current file position will do.
                 *
                 * Note:
                 *  - The current position in the file is at the end of the stretched file due to the call to lseek().
                 *  - An empty string is actually a single '\0' character, so a zero-byte will be written at the last byte of the file.
                 */
                res = write(_resilienceTreeFileDescriptor, "", 1);
                if (res != 1) {
                    close( _resilienceTreeFileDescriptor );
                    fatal0( "Error resizing file" );
                }
            }

            //Now the file is ready to be mmaped.
            _resilienceTree = ( ResilienceNode * ) mmap( 0, _RESILIENCE_MAX_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, _resilienceTreeFileDescriptor, 0 );
            if( _resilienceTree == MAP_FAILED ) {
                close( _resilienceTreeFileDescriptor );
                fatal0( "Error mmaping file" );
            }
        }

        //Resilience results mmapped file must be created before any WD because in WD constructor we will use the memory mapped.
        if( _resilienceResults == NULL ) {
            // Check if we have a file with the resilience tree from past executions.
            _resilienceResultsFileDescriptor = open( _resilienceResultsFilepath, O_RDWR );

            //If there isn't a file from past executions.
            if( _resilienceResultsFileDescriptor == -1 ) {
                _resilienceResultsFileDescriptor = open( _resilienceResultsFilepath, O_RDWR | O_CREAT, (mode_t) 0600 );
                if( _resilienceResultsFileDescriptor == -1 )
                    fatal0( "Resilience: Error creating persistentTree." );

                //Stretch file.
                int res = lseek( _resilienceResultsFileDescriptor, _RESILIENCE_MAX_FILE_SIZE - 1, SEEK_SET );
                if( res == -1 ) {
                    close( _resilienceResultsFileDescriptor );
                    fatal0( "Resilience: Error calling lseek." );
                }

                /* Something needs to be written at the end of the file to force the file have actually the new size.
                 * Just writing an empty string at the current file position will do.
                 *
                 * Note:
                 *  - The current position in the file is at the end of the stretched file due to the call to lseek().
                 *  - An empty string is actually a single '\0' character, so a zero-byte will be written at the last byte of the file.
                 */
                res = write(_resilienceResultsFileDescriptor, "", 1);
                if (res != 1) {
                    close( _resilienceResultsFileDescriptor );
                    fatal0( "Error resizing file" );
                }
            }

            //Now the file is ready to be mmaped.
            _resilienceResults = ( ResilienceNode * ) mmap( 0, _RESILIENCE_MAX_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, _resilienceResultsFileDescriptor, 0 );
            if( _resilienceResults == MAP_FAILED ) {
                close( _resilienceResultsFileDescriptor );
                fatal0( "Error mmaping file" );
            }
            _freeResilienceResults.insert( std::make_pair( 0, _RESILIENCE_MAX_FILE_SIZE ) );
        }

        //Update _freeResilienceResults with the restored tree.

        size_t treeSize = _RESILIENCE_MAX_FILE_SIZE / sizeof( ResilienceNode );
        if( restoreTree ) {
            for( unsigned int i = 0; i < treeSize; i++ ) {
                ResilienceNode * rn = &_resilienceTree[i];
                if( rn->isInUse() ) {
                    _usedResilienceNodes.push_back( i );
                    if( rn->isComputed() ) {
                        restoreResilienceResultsSpace( rn->getResultIndex(), rn->getResultSize() );
                        if( rn->getNumDescendants() > 0 ) {
                            /* If RN is computed it shouldn't have descs, so it's necessary remove them. 
                             * However, it's not possible to use ResilienceNode::removeAllDescs() because
                             * this method uses sys._resilience which is being created here.
                             */
                            ResilienceNode * current = sys.getResiliencePersistence()->getResilienceNode( rn->_desc );
                            while( current != NULL ) {
                                int toDelete = rn->_desc;
                                current->removeAllDescs();
                                rn->_desc = current->_next;
                                current = getResilienceNode( current->_next );
                                freeResilienceNode( toDelete );
                                rn->_descSize--;
                            }

                            if( rn->_descSize != 0 || rn->_desc != 0 )
                                fatal0( "There are still descs" );
                        }
                    }
                    rn->restartLastDescRestored();
                }
                else {
                    _freeResilienceNodes.push( i );
                }
            }
        }
        else {
            for( unsigned int i = 0; i < treeSize; i++)
                _freeResilienceNodes.push( i );
        }

        // JUST FOR DEBUG PURPOSES
        //std::cerr << "-------------------- EXECUTION START --------------------" << std::endl;
        size_t free_results = 0;
        for( std::map<unsigned int, size_t>::iterator it = _freeResilienceResults.begin();
                it != _freeResilienceResults.end();
                it++ )
        {
            free_results += it->second;
            //std::cerr << "There are " << it->second << " bytes free starting at results[" << it->first << "]." << std::endl;
        }
        //std::cerr << "There are " << free_results << " bytes free in results." << std::endl;
        //std::cerr << "There are " << _RESILIENCE_MAX_FILE_SIZE - free_results << " bytes used in results." << std::endl;
        //std::cerr << "There are " << _usedResilienceNodes.size() << " resilience nodes in use." << std::endl;
        //std::cerr << "-------------------- EXECUTION START --------------------" << std::endl;
    }

    ResiliencePersistence::~ResiliencePersistence() {
        //Free mapped file for resilience tree.
        int res = munmap( _resilienceTree, _RESILIENCE_MAX_FILE_SIZE );
        if( res == -1 )
            fatal( "Error unmapping file." );
        close( _resilienceTreeFileDescriptor );

        //Free mapped file for resilience results.
        res = munmap( _resilienceResults, _RESILIENCE_MAX_FILE_SIZE );
        if( res == -1 )
            fatal( "Error unmapping file." );
        close( _resilienceResultsFileDescriptor );
    }

    /********** RESILIENCE PERSISTENCE **********/
    

    /********** RESILIENCE NODE **********/

    bool ResilienceNode::addNext( int next ) {
        if( _next == 0 ) {
            _next = next; 
            return true;
        }

        int cont = 0;

        ResilienceNode * current = sys.getResiliencePersistence()->getResilienceNode( _next );
        while( current->_next != 0 ) {
            current = sys.getResiliencePersistence()->getResilienceNode( current->_next );
            //cont++;
            //if( cont == 500 ) fatal0( "There is a cycle in the resilience tree." ); 
        }
        current->_next = next; 
        return true;
    }


    void ResilienceNode::storeResult( CopyData * copies, size_t numCopies, int task_id ) {
        //Calculate outputs size
        size_t outputs_size = 0;
        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isOutput() ) {
                outputs_size += copies[i].getDimensions()->accessed_length;
            }
        }

        if( outputs_size <= 0 )
            return;
            //fatal( "Store result of 0 bytes makes no sense." );

        //Get result from resilience results mmaped file.
        _resultSize = outputs_size;
        _resultIndex = ( char * )sys.getResiliencePersistence()->getResilienceResultsFreeSpace( _resultSize ) 
            - ( char * )sys.getResiliencePersistence()->getResilienceResults( 0 );
        //Copy the result
        char * aux = ( char * ) sys.getResiliencePersistence()->getResilienceResults( _resultIndex );
        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isOutput() ) {
                size_t copy_size = copies[i].getDimensions()->accessed_length;
                void * copy_address = ( char * ) copies[i].getBaseAddress() + copies[i].getOffset();
                memcpy( aux, copy_address, copy_size );
                aux += copy_size;
            }
        }

        //Mark it as computed
        _computed = true;

        //Remove all descendents. They are not needed anymore.
        removeAllDescs();
    }

    void ResilienceNode::loadResult( CopyData * copies, size_t numCopies, int task_id ) { 
        char * aux = ( char * ) sys.getResiliencePersistence()->getResilienceResults( _resultIndex );
        for( unsigned int i = 0; i < numCopies; i++ ) {
            if( copies[i].isOutput() ) {
                size_t copy_size = copies[i].getDimensions()->accessed_length;
                void * copy_address = ( char * ) copies[i].getBaseAddress() + copies[i].getOffset();
                memcpy( copy_address, aux, copy_size );
                aux += copy_size;
            }
        }
    }

    ResilienceNode* ResilienceNode::getNextDescToRestore() {
        //TODO: FIXME: Maybe, this should throw FATAL ERROR.
        if( _desc == 0 || _lastDescRestored >= _descSize ) {
            _lastDescRestored++;
            return NULL;
        }

        ResilienceNode * desc = sys.getResiliencePersistence()->getResilienceNode( _desc );
        for( unsigned int i = 0; i < _lastDescRestored; i++) {
            //TODO: FIXME: Maybe, this should throw FATAL ERROR.
            if( desc->_next == 0 ) {
                _lastDescRestored++;
                return NULL;
            }
            desc = sys.getResiliencePersistence()->getResilienceNode( desc->_next );
        }

        if( desc != NULL )
            _lastDescRestored++;

        return desc;
    }

    bool ResilienceNode::addDesc( int desc ) { 
        if( desc == 0 )
            fatal0( "Trying to add null descendent." ); 

        if( _desc == 0 ) 
            _desc = desc; 
        else
            sys.getResiliencePersistence()->getResilienceNode( _desc )->addNext( desc );

        _descSize++;
        return true;
    }

    void ResilienceNode::removeAllDescs() {
        if( _descSize == 0 ) 
            return;

        ResilienceNode * current = sys.getResiliencePersistence()->getResilienceNode( _desc );
        while( current != NULL ) {
            int toDelete = _desc;
            current->removeAllDescs();
            _desc = current->_next;
            current = sys.getResiliencePersistence()->getResilienceNode( current->_next );
            sys.getResiliencePersistence()->freeResilienceNode( toDelete );
            _descSize--;
        }

        if( _descSize != 0 || _desc != 0 )
            fatal0( "There are still descs" );
    }

    /********** RESILIENCE NODE **********/

}
