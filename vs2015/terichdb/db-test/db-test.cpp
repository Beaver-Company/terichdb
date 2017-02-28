// db-test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <terark/terichdb/db_table.hpp>
#include <terark/io/DataIO.hpp>
#include <terark/io/MemStream.hpp>
#include <terark/io/RangeStream.hpp>
#include <terark/num_to_str.hpp>
#include <thread>
#include <random>
#include "TestRow.hpp"

using namespace DbTest;
using namespace terark::terichdb;


void doTest(const char* tableDir, size_t maxRowNum)
{
    using namespace terark;

    std::mt19937 mt;
    DbTablePtr tab = TestRow::openTable(tableDir, true);
    DbContextPtr ctx = tab->createDbContext();

    valvec<llong> recIdvec;
    valvec<byte_t>  recBuf;
    NativeDataOutput<AutoGrownMemIO> rowBuilder;
    TestRow recRow;

    llong insertedRows = 0;
    febitvec bits(maxRowNum + 1);
    for(size_t i = 0; i < maxRowNum; ++i)
    {
        TestRow recRow;
        recRow.id = std::uniform_int_distribution<size_t>(1, maxRowNum)(mt);
        int len = sprintf(recRow.fix.data, "%06lld", llong(recRow.id));
        recRow.str0.assign("s0:").append(recRow.fix.data, len);
        recRow.str1.assign("s1:").append(recRow.fix.data, len);
        recRow.str2.assign("s2:").append(recRow.fix.data, len);
        recRow.str3.assign("s3:").append(recRow.fix.data, len);
        recRow.str4.assign("s4:").append(recRow.fix.data, len);
        sprintf(recRow.fix2.data, "F2.%06lld", llong(recRow.id));
        fstring binRow = recRow.encode(rowBuilder);
        if(bits[recRow.id])
        {
            if(!ctx->indexKeyExists(0, Schema::fstringOf(&recRow.id)))
            {
                i = i;
            }
            printf("dupkey: %s\n", tab->rowSchema().toJsonStr(binRow).c_str());
            ctx->indexSearchExact(0, Schema::fstringOf(&recRow.id), &recIdvec);
            assert(recIdvec.size() > 0);
        }
        if(600 == i)
            i = i;
        if(3903 == recRow.id)
            i = i;
        llong recId = ctx->insertRow(binRow);
        if(recId < 0)
        {
            assert(bits.is1(recRow.id));
            printf("Insert failed: %s\n", ctx->errMsg.c_str());
        }
        else
        {
            ctx->getValue(recId, &recBuf);
            std::string js1 = tab->toJsonStr(binRow);
            printf("Insert recId = %lld: %s\n", recId, js1.c_str());
            if(binRow != recBuf)
            {
                std::string js2 = tab->toJsonStr(recBuf);
                printf("Fetch_ recId = %lld: %s\n", recId, js2.c_str());
                assert(0);
            }
            ctx->indexSearchExact(0, Schema::fstringOf(&recRow.id), &recIdvec);
            assert(recIdvec.size() > 0);
            insertedRows++;
            if(bits.is1(recRow.id))
            {
                ctx->removeRow(recId);
                llong recId2 = ctx->insertRow(binRow);
                assert(recId2 == recId);
            }
            assert(tab->exists(recId));
            assert(bits.is0(recRow.id));
        }
        bits.set1(recRow.id);

        if(std::uniform_real_distribution<float>(0, 1)(mt) < 0.3f)
        {
            llong randomRecId = std::uniform_int_distribution<size_t>(0, tab->numDataRows() - 1)(mt);
            if(22 == randomRecId)
                i = i;
            uint64_t keyId = 0;
            recBuf.erase_all();
            std::string jstr;
            if(tab->exists(randomRecId))
            {
                size_t indexId = tab->getIndexId("id");
                assert(indexId < tab->getIndexNum());
                ctx->selectOneColumn(randomRecId, indexId, &recBuf);
                keyId = unaligned_load<uint64_t>(recBuf.data());
                if(keyId == 20538)
                    keyId = keyId;
                ctx->getValue(randomRecId, &recBuf);
                jstr = tab->toJsonStr(recBuf);
                assert(keyId > 0);
                //	assert(bits.is1(keyId));
            }
            bool isDeleted = false;
            if(std::uniform_real_distribution<float>(0, 1)(mt) < 0.3f)
            {
                // may remove deleted record
                ctx->removeRow(randomRecId);
                assert(!tab->exists(randomRecId));
                assert(!ctx->indexKeyExists(0, Schema::fstringOf(&keyId)));
                isDeleted = true;
            }
            else if(tab->exists(randomRecId))
            {
                ctx->removeRow(randomRecId);
                assert(!tab->exists(randomRecId));
                assert(!ctx->indexKeyExists(0, Schema::fstringOf(&keyId)));
                isDeleted = true;
            }
            if(isDeleted && keyId > 0)
            {
                printf("delete success: recId = %lld: %s\n"
                       , randomRecId, jstr.c_str());
                assert(!tab->exists(randomRecId));
                bits.set0(keyId);
            }
        }

        if(std::uniform_real_distribution<float>(0, 1)(mt) < 0.3f)
        {
            llong randomRecId = std::uniform_int_distribution<size_t>(0, tab->numDataRows() - 1)(mt);
            if(tab->exists(randomRecId))
            {
                size_t keyId_ColumnId = 0;
                decltype(TestRow::fix2) fix2;
                ctx->selectOneColumn(randomRecId, keyId_ColumnId, &recBuf);
                assert(recBuf.size() == sizeof(llong));
                llong keyId = Schema::numberOf<llong>(recBuf);
                int len = sprintf(fix2.data, "F-%lld", keyId);
                TERARK_RT_assert(len < (int)sizeof(fix2.data), std::out_of_range);
                tab->updateColumn(randomRecId, "fix2", fix2);
            }
        }

        if(std::uniform_real_distribution<float>(0, 1)(mt) < 0.001f)
        {
            //tab->compact();
        }
    }

    {
        valvec<byte_t> keyHit, val;
        valvec<llong> idvec;
        for(size_t indexId = 0; indexId < tab->getIndexNum(); ++indexId)
        {
            IndexIteratorPtr indexIter = tab->createIndexIterForward(indexId, ctx.get());
            const Schema& indexSchema = tab->getIndexSchema(indexId);
            valvec<byte_t> keyData;
            for(size_t i = 0; i < maxRowNum / 5; ++i)
            {
                llong keyInt = std::uniform_int_distribution<size_t>(0, maxRowNum * 11 / 10)(mt);
                char keyBuf[64];
                switch(indexId)
                {
                default:
                    assert(0);
                    break;
                case 0:
                    keyData.assign(Schema::fstringOf(&keyInt));
                    break;
                case 1: // str0
                    keyData.assign(keyBuf, sprintf(keyBuf, "s0:%06lld", keyInt));
                    break;
                case 2: // str1
                    keyData.assign(keyBuf, sprintf(keyBuf, "s1:%06lld", keyInt));
                    break;
                case 3: // str2
                    keyData.assign(keyBuf, sprintf(keyBuf, "s2:%06lld", keyInt));
                    break;
                case 4: // fix
                    assert(indexSchema.getFixedRowLen() > 0);
                    keyData.assign(keyBuf, sprintf(keyBuf, "%06lld", keyInt));
                    keyData.resize(indexSchema.getFixedRowLen());
                    break;
                case 5: // str0,str1
                    keyData.assign(keyBuf, sprintf(keyBuf, "s0:%06lld", keyInt));
                    keyData.push_back('\0');
                    keyData.append(keyBuf, sprintf(keyBuf, "s1:%06lld", keyInt));
                    break;
                }
                keyData.push_back('\0'); keyData.pop_back(); // add an extra '\0'
                idvec.resize(0);
                std::string keyJson = indexSchema.toJsonStr(keyData);
                printf("find index key = %s", keyJson.c_str());
                fflush(stdout);
                llong recId;
                if(i == 0x002d && indexId == 1)
                    i = i;
                int ret = indexIter->seekLowerBound(keyData, &recId, &keyHit);
                if(ret > 0)
                {
                    printf(", found upper_bound key=%s, recId=%lld\n",
                           indexSchema.toJsonStr(keyHit).c_str(), recId);
                    //	printf(", found hitkey > key, show first upper_bound:\n");
                    //	idvec.push_back(recId);
                }
                else if(ret < 0)
                { // all keys are less than search key
                    printf(", all keys are less than search key\n");
                }
                else if(ret == 0)
                { // found exact key
                    idvec.push_back(recId);
                    int hasNext; // int as bool
                    while((hasNext = indexIter->increment(&recId, &keyHit))
                          && fstring(keyHit) == keyData)
                    {
                        assert(recId < tab->numDataRows());
                        idvec.push_back(recId);
                    }
                    if(hasNext)
                        idvec.push_back(recId);
                    printf(", found %zd exact and %d upper_bound\n",
                           idvec.size() - hasNext, hasNext);
                }
                for(size_t i = 0; i < idvec.size(); ++i)
                {
                    recId = idvec[i];
                    ctx->getValue(recId, &val);
                    printf("%8lld  | %s\n", recId, tab->toJsonStr(val).c_str());
                }
            }
        }
    }

    {
        printf("test iterate table, numDataRows=%lld ...\n", tab->numDataRows());
        StoreIteratorPtr storeIter = ctx->createTableIterForward();
        llong recId;
        valvec<byte_t> val;
        llong iterRows = 0;
        while(storeIter->increment(&recId, &val))
        {
            printf("%8lld  | %s\n", recId, tab->toJsonStr(val).c_str());
            ++iterRows;
        }
        printf("test iterate table passed, iterRows=%lld, insertedRows=%lld\n",
               iterRows, insertedRows);
    }

    // last writable segment will put to compressing queue
    tab->syncFinishWriting();
}

void doTestMultiThread(const char* tableDir, size_t maxRowNum, size_t threadNum)
{
    using namespace terark;

    DbTablePtr tab = TestRow::openTable(tableDir, true);
    auto test = [&](size_t id)
    {
        DbContextPtr ctx = tab->createDbContext();
        std::mt19937 mt(id);
        valvec<llong> recIdvec;
        valvec<byte_t>  recBuf;
        NativeDataOutput<AutoGrownMemIO> rowBuilder;
        TestRow recRow;

        for(size_t i = 0; i < maxRowNum; ++i)
        {
            recRow.id = std::uniform_int_distribution<size_t>(1, maxRowNum)(mt);
            int len = sprintf(recRow.fix.data, "%06lld", llong(recRow.id));
            recRow.str0.assign("s0:").append(recRow.fix.data, len);
            recRow.str1.assign("s1:").append(recRow.fix.data, len);
            recRow.str2.assign("s2:").append(recRow.fix.data, len);
            recRow.str3.assign("s3:").append(recRow.fix.data, len);
            recRow.str4.assign("s4:").append(recRow.fix.data, len);
            sprintf(recRow.fix2.data, "F2.%06lld", llong(recRow.id));
            fstring binRow = recRow.encode(rowBuilder);
            try
            {
                ctx->upsertRow(binRow);
            }
            catch(NeedRetryException const &)
            {
            }
        }
    };
    std::vector<std::thread> thread_vec;
    for(size_t i = 1; i < threadNum; ++i)
    {
        thread_vec.emplace_back(test, i);
    }
    test(0);
    for(auto &t : thread_vec)
    {
        t.join();
    }

    // last writable segment will put to compressing queue
    //tab->compact();
    tab->syncFinishWriting();
}

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "usage: %s maxRowNum\n", argv[0]);
        return 1;
    }
    size_t maxRowNum = (size_t)strtoull(argv[1], NULL, 10);
    doTestMultiThread("dfadb", maxRowNum, 4);
    //doTest("dfadb", maxRowNum);
    DbTable::safeStopAndWaitForCompress();
    return 0;
}
