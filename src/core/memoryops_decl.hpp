#ifndef MEMORYOPS_DECL
#define MEMORYOPS_DECL

#include "addressspace_decl.hpp"
#include "memcachecopy_fwd.hpp"
namespace nanos {
class BaseOps {
   public:
   // define la transferencia de la version _version de una region _reg 
   // posible merge OwnOp con TransferListEntry? cuidado, porque puede haber transfer list con entradas de operaciones que está realizando otro thread (no existe ownop pero si device ops)
   struct OwnOp {
      DeviceOps         *_ops;//
      global_reg_t       _reg;// la region que se quiere copiar
      unsigned int       _version;
      memory_space_id_t  _location;// el origen de donde se copiara el dato
      OwnOp( DeviceOps *ops, global_reg_t reg, unsigned int version, memory_space_id_t location );// constructor
      OwnOp( OwnOp const &op );// constructor copia
      OwnOp &operator=( OwnOp const &op );// asignacion
      // comparacion para poder meterlo en un set
      // utiliza puntero _ops porque para dos global_reg_t iguales,
      // el valor de *_ops es el mismo (al fin y al cabo se obtiene de la cache o el directorio usando el global_reg_t)
      bool operator<( OwnOp const &op ) const {
         return ( ( uintptr_t ) _ops ) < ( ( uintptr_t ) op._ops );
      }
      // actualiza la localizacion y la version de la region una
      // vez la transferencia se ha completado
      void commitMetadata( ProcessingElement *pe ) const;
   };
   private:
   // indica que las transferencias no se confirman hasta que todas las operaciones han terminado
   // a lo mejor merece la pena meterlo dentro de la politica tambien...
   bool                     _delayedCommit;
   // usado para que el isDataReady no compruebe todo una vez ya esta comprobado
   bool                     _dataReady;
   // a veces puede ser null (no es obligatorio)
   // cuando no es nulo, se añade el processing element al conjunto de PEs de la entrada del diccionario que está en el directorio (para data locality).
   ProcessingElement       *_pe;
   // Conjunto de transferencias gestionadas por este objeto
   std::set< OwnOp >        _ownDeviceOps;
   // "Operaciones que estas esperando pero que las hace otro workdescriptor"
   // Por ejemplo: cuando dos workdescriptors tienen el mismo in y quieren hacer la copia hacia el mismo destino.
   std::set< DeviceOps * >  _otherDeviceOps;
   // Deprecated? Se utiliza para informacion
   std::size_t              _amountOfTransferredData;

   BaseOps( BaseOps const &op );
   BaseOps &operator=( BaseOps const &op );
   protected:
   // Chunks de origen que se bloquearán para evitar invalidaciones
   std::set< AllocatedChunk * > _lockedChunks;

   public:
   BaseOps( ProcessingElement *pe, bool delayedCommit );
   ~BaseOps();
   // getter y setters
   ProcessingElement *getPE() const;
   std::set< DeviceOps * > &getOtherOps();
   std::set< OwnOp > &getOwnOps();
   std::size_t getAmountOfTransferredData() const;
   void addAmountTransferredData(std::size_t amount);

   // añade una nueva transferencia y actualiza el directorio si no es _delayedCommit
   void insertOwnOp( DeviceOps *ops, global_reg_t reg, unsigned int version, memory_space_id_t location );

   // informa de que la transferencia ha terminado
   // el flag inval no se utiliza actualmente para nada
   bool isDataReady( WD const &wd, bool inval = false );

   // desbloquea los chunks bloqueados para que puedan ser invalidados
   // el workdescriptor es para debug de las invalidaciones
   void releaseLockedSourceChunks( WD const &wd );
};


// used when the task is executed in the host and the data has to be retrieved from one or multiple devices
class BaseAddressSpaceInOps : public BaseOps {
   protected:
   typedef std::map< SeparateMemoryAddressSpace *, TransferList > MapType;
   // classifies transferences by source.
   MapType _separateTransfers;

   public:
   BaseAddressSpaceInOps( ProcessingElement *pe, bool delayedCommit );
   virtual ~BaseAddressSpaceInOps();

   // 
   void addOp( SeparateMemoryAddressSpace *from, global_reg_t const &reg, unsigned int version, AllocatedChunk *chunk, unsigned int copyIdx );
   void lockSourceChunks( global_reg_t const &reg, unsigned int version, NewLocationInfoList const &locations, memory_space_id_t thisLocation, WD const &wd, unsigned int copyIdx );

   // por que se usa esto aqui? host to host tiene sentido???
   virtual void addOpFromHost( global_reg_t const &reg, unsigned int version, AllocatedChunk *chunk, unsigned int copyIdx );

   // workdescriptor se usa por temas de debugging
   virtual void issue( WD const &wd );

   // workdescriptor y copyindex se usa por temas de debugging
   virtual unsigned int getVersionNoLock( global_reg_t const &reg, WD const &wd, unsigned int copyIdx );

   // Bloquea los AllocatedChunks de origen para evitar invalidaciones.
   // "Añade una operacion como pendiente" en el objeto de sincronizacion DeviceOps
   // Busca en el directorio las regiones que serviran de origen
   // [...]
   // Nota: tanto WD como copyIdx se utilizan para debug
   virtual void copyInputData( MemCacheCopy const &memCopy, WD const &wd, unsigned int copyIdx );

   // reserva memoria necesaria para almacenar los datos de output (no inout)
   virtual void allocateOutputMemory( global_reg_t const &reg, unsigned int version, WD const &wd, unsigned int copyIdx );
};

typedef BaseAddressSpaceInOps HostAddressSpaceInOps;

class SeparateAddressSpaceInOps : public BaseAddressSpaceInOps {
   protected:
   // a donde se copiaran las cosas
   SeparateMemoryAddressSpace &_destination;
   // ademas de transferencias que provienen de otros dispositivos 
   // (declaradas en BaseAddressSpaceInOps), tambien nos interesan
   // las transferencias cuyo origen es el host
   TransferList _hostTransfers;

   public:
   SeparateAddressSpaceInOps( ProcessingElement *pe, bool delayedCommit, SeparateMemoryAddressSpace &destination );
   ~SeparateAddressSpaceInOps();

   virtual void addOpFromHost( global_reg_t const &reg, unsigned int version, AllocatedChunk *chunk, unsigned int copyIdx );

   // workdescriptor se usa para debug
   virtual void issue( WD const &wd );

   // Nota: tanto WD como copyIdx se utilizan para debug
   virtual unsigned int getVersionNoLock( global_reg_t const &reg, WD const &wd, unsigned int copyIdx );

   // Nota: tanto WD como copyIdx se utilizan para debug
   virtual void copyInputData( MemCacheCopy const &memCopy, WD const &wd, unsigned int copyIdx );

   // Nota: tanto WD como copyIdx se utilizan para debug
   virtual void allocateOutputMemory( global_reg_t const &reg, unsigned int version, WD const &wd, unsigned int copyIdx );
};

// Gestiona transferencias que sacan datos de dispositivo/s hacia el host
// Nota: pueden ser varios dispositivos. 
// Cuando se hace un taskwait y se han ejecutado tareas en dispositivos diferentes, se han de enviar todos esos datos al host.
// Cuando se utiliza la politica WriteThrough y NoCache, se hace una copia al host nada mas terminar la tarea
class SeparateAddressSpaceOutOps : public BaseOps {
   typedef std::map< SeparateMemoryAddressSpace *, TransferList > MapType;
   bool _invalidation;
   MapType _transfers;

   public:
   SeparateAddressSpaceOutOps( ProcessingElement *pe, bool delayedCommit, bool isInval );
   ~SeparateAddressSpaceOutOps();

   // Nota: tanto WD como copyIdx se utilizan para debug
   void addOp( SeparateMemoryAddressSpace *from, global_reg_t const &reg, unsigned int version, DeviceOps *ops, AllocatedChunk *chunk, WD const &wd, unsigned int copyIdx );

   // Nota: tanto WD como copyIdx se utilizan para debug
   void addOp( SeparateMemoryAddressSpace *from, global_reg_t const &reg, unsigned int version, DeviceOps *ops, WD const &wd, unsigned int copyIdx );

   // Nota: WD se utiliza para debug
   void issue( WD const &wd );

   // Nota: tanto WD como copyIdx se utilizan para debug
   void copyOutputData( SeparateMemoryAddressSpace *from, MemCacheCopy const &memCopy, bool output, WD const &wd, unsigned int copyIdx );
};

}

#endif /* MEMORYOPS_DECL */
