#include <Storages/MergeTree/MergeTreeBlockOutputStream.h>
#include <Storages/MergeTree/MergeTreeDataPartInMemory.h>
#include <Storages/StorageMergeTree.h>
#include <Interpreters/PartLog.h>


namespace DB
{

Block MergeTreeBlockOutputStream::getHeader() const
{
    return metadata_snapshot->getSampleBlock();
}


void MergeTreeBlockOutputStream::write(const Block & block)
{
    storage.delayInsertOrThrowIfNeeded();

    auto part_blocks = storage.writer.splitBlockIntoParts(block, max_parts_per_block, metadata_snapshot);
    for (auto & current_block : part_blocks)
    {
        Stopwatch watch;

        MergeTreeData::MutableDataPartPtr part = storage.writer.writeTempPart(current_block, metadata_snapshot, optimize_on_insert);
        storage.renameTempPartAndAdd(part, &storage.increment);

        PartLog::addNewPart(storage.global_context, part, watch.elapsed());

        /// Initiate async merge - it will be done if it's good time for merge and if there are space in 'background_pool'.
        storage.background_executor.triggerTask();
    }
}

}
