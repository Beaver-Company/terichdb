// terichdb-schema-compile.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define TERICHDB_SCHEMA_COMPILER
#include <terark/terichdb/db_conf.cpp>

#if defined(_WIN32) || defined(_WIN64)
#include <getopt.c>
#endif

using namespace std;
using namespace terark;
using namespace terark::terichdb;

int maxColnameLen(const Schema& schema) {
	const size_t colnum = schema.columnNum();
	int maxNameLen = 0;
	for (size_t i = 0; i < colnum; ++i) {
		const fstring colname = schema.getColumnName(i);
		maxNameLen = std::max(maxNameLen, colname.ilen());
	}
	return maxNameLen;
}

void compileOneSchema(const Schema& schema, const char* className,
					  const char* rowClassName) {
	const size_t colnum = schema.columnNum();
	printf("\n");
	printf("  struct %s {\n", className);
	for (size_t i = 0; i < colnum; ++i) {
		const ColumnMeta& colmeta = schema.getColumnMeta(i);
		const fstring     colname = schema.getColumnName(i);
		switch (colmeta.type) {
		case ColumnType::Any:
			assert(0); // not supported now
			break;
		case ColumnType::Nested:
			assert(0); // not supported now
			break;
		case ColumnType::Uint08:
			printf("    unsigned char %s;\n", colname.p);
			break;
		case ColumnType::Sint08:
			printf("    signed char %s;\n", colname.p);
			break;
		case ColumnType::Uint16:
			printf("    std::uint16_t %s;\n", colname.p);
			break;
		case ColumnType::Sint16:
			printf("    std::int16_t %s;\n", colname.p);
			break;
		case ColumnType::Uint32:
			printf("    std::uint32_t %s;\n", colname.p);
			break;
		case ColumnType::Sint32:
			printf("    std::int32_t %s;\n", colname.p);
			break;
		case ColumnType::Uint64:
			printf("    std::uint64_t %s;\n", colname.p);
			break;
		case ColumnType::Sint64:
			printf("    std::int64_t %s;\n", colname.p);
			break;
		case ColumnType::Uint128:
			printf("    unsigned __int128 %s;\n", colname.p);
			break;
		case ColumnType::Sint128:
			printf("    signed __int128 %s;\n", colname.p);
			break;
		case ColumnType::Float32:
			printf("    float %s;\n", colname.p);
			break;
		case ColumnType::Float64:
			printf("    double %s;\n", colname.p);
			break;
		case ColumnType::Float128:
			printf("    __float128 %s;\n", colname.p);
			break;
		case ColumnType::Decimal128:
			printf("    __decimal128 %s;\n", colname.p);
			break;
		case ColumnType::Uuid:
			printf("    terark::terichdb::Schema::Fixed<16> %s;\n", colname.p);
			break;
		case ColumnType::Fixed:
			if (colmeta.realtype.empty()) {
				printf("    terark::terichdb::Schema::Fixed<%d> %s;\n", int(colmeta.fixedLen), colname.p);
			}
			else {
				const char* realtype = colmeta.realtype.c_str();
				const int fixlen = int(colmeta.fixedLen);
				printf(R"EOS(
    %s %s;
 // dumpable type does not require sizeof(T)==fixlen, it only requires that
 // dump_size(T)==fixlen, but check for dump_size(T)==fixlen is cumbersome
 // and requires big changes for terark.dataio
 // so we static assert sizeof(T)==fixlen here:
    BOOST_STATIC_ASSERT(sizeof(%s) == %d);
    BOOST_STATIC_ASSERT((terark::DataIO_is_dump<terark::NativeDataInput<terark::MemIO>, %s >::value));

)EOS", realtype, colname.p, realtype, fixlen, realtype);
			}
			break;
		case ColumnType::VarSint:
			printf("    terark::var_int64_t %s;\n", colname.p);
			break;
		case ColumnType::VarUint:
			printf("    terark::var_uint64_t %s;\n", colname.p);
			break;
		case ColumnType::StrZero:
			printf("    std::string %s;\n", colname.p);
			break;
		case ColumnType::TwoStrZero:
			printf("    terark::terichdb::Schema::TwoStrZero %s;\n", colname.p);
			break;
		case ColumnType::Binary:
			if (colmeta.realtype.empty()) {
				printf("    std::string %s;\n", colname.p);
			}
			else {
				const char* realtype = colmeta.realtype.c_str();
				printf("    %s %s;\n", realtype, colname.p);
			}
			break;
		case ColumnType::CarBin:
			if (colmeta.realtype.empty()) {
				printf("    terark::terichdb::Schema::CarBin %s;\n", colname.p);
			}
			else {
				const char* realtype = colmeta.realtype.c_str();
				printf("    %s %s;\n", realtype, colname.p);
			}
			break;
		}
	}
	printf("\n");
	printf("    DATA_IO_LOAD_SAVE(%s,\n", className);
	for (size_t i = 0; i < colnum; ++i) {
		const ColumnMeta& colmeta = schema.getColumnMeta(i);
		const fstring     colname = schema.getColumnName(i);
		switch (colmeta.type) {
		case ColumnType::Any:
			assert(0); // not supported now
			break;
		case ColumnType::Nested:
			assert(0); // not supported now
			break;
		case ColumnType::Uint08:
		case ColumnType::Sint08:
		case ColumnType::Uint16:
		case ColumnType::Sint16:
		case ColumnType::Uint32:
		case ColumnType::Sint32:
		case ColumnType::Uint64:
		case ColumnType::Sint64:
		case ColumnType::Uint128:
		case ColumnType::Sint128:
		case ColumnType::Float32:
		case ColumnType::Float64:
		case ColumnType::Float128:
		case ColumnType::Decimal128:
		case ColumnType::Uuid:
		case ColumnType::Fixed:
		case ColumnType::VarSint:
		case ColumnType::VarUint:
			printf("      &%s\n", colname.p);
			break;
		case ColumnType::StrZero:
			if (i < colnum-1) {
				printf("      &terark::terichdb::Schema::StrZero(%s)\n", colname.p);
			} else {
				printf("      &terark::RestAll(%s)\n", colname.p);
			}
			break;
		case ColumnType::TwoStrZero:
		case ColumnType::Binary:
		Case_Binary:
			if (i < colnum-1) {
				printf("      &%s\n", colname.p);
			} else {
				printf("      &terark::RestAll(%s)\n", colname.p);
			}
			break;
		case ColumnType::CarBin:
			if (colmeta.realtype.empty()) {
				goto Case_Binary;
			}
			if (i < colnum-1) {
				// read carbin size and check carbin size with real size
				printf("      &terark::terichdb::Schema::CarBinPack(%s)\n", colname.p);
			} else {
				printf("      &%s\n", colname.p);
			}
			break;
		}
	}
	printf("    )\n");
	printf(
R"EOS(
    %s& decode(terark::fstring ___row) {
      terark::NativeDataInput<terark::MemIO> ___dio(___row.range());
      ___dio >> *this;
      return *this;
    }
    terark::fstring
    encode(terark::NativeDataOutput<terark::AutoGrownMemIO>& ___dio) const {
      ___dio.rewind();
      ___dio << *this;
      return ___dio.written();
    }
)EOS", className
	);
	if (fstring(className) == rowClassName) {
		printf(
R"EOS(
    // DbTablePtr use none-const ref is just for ensure application code:
    // var 'tab' must be a 'DbTablePtr', can not be a 'DbTable*'
    static bool
    checkTableSchema(terark::terichdb::DbTablePtr& tab, bool checkColname = false);

    static terark::terichdb::DbTablePtr
    openTable(const boost::filesystem::path& dbdir, bool checkColname = false) {
      using namespace terark::terichdb;
      DbTablePtr tab = DbTable::open(dbdir);
      if (!checkTableSchema(tab, checkColname)) {
        THROW_STD(invalid_argument,
          "database schema is inconsistence with compiled c++ code, dbdir: %%s",
          dbdir.string().c_str());
      }
      return tab;
    }
)EOS");
	}
	else {
		int maxNameLen = maxColnameLen(schema);
		printf("    %s& select(const %s& ___row) {\n", className, rowClassName);
		for (size_t i = 0; i < colnum; ++i) {
			const char* colname = schema.getColumnName(i).c_str();
			printf("      %-*s = ___row.%s;\n", maxNameLen, colname, colname);
		}
		printf("      return *this;\n"); // select
		printf("    }\n"); // select
		printf("    void assign_to(%s& ___row) const {\n", rowClassName);
		for (size_t i = 0; i < colnum; ++i) {
			const char* colname = schema.getColumnName(i).c_str();
			printf("      ___row.%-*s = %s;\n", maxNameLen, colname, colname);
		}
		printf("    }\n"); // select
	}
#if 0
	printf(
R"EOS(
    static const terark::terichdb::Schema& getSchema() {
      using namespace terark::terichdb;
      static Schema schema;
      // static std::mutex mtx;
      // std::unique_lock<std::mutex> lock(mtx);
      if (schema.columnNum() == 0) {
)EOS");
	for (size_t i = 0; i < colnum; ++i) {
		const char* colname = schema.getColumnName(i).c_str();
		const auto& colmeta = schema.getColumnMeta(i);
		printf(
R"EOS(        {
          ColumnMeta colmeta(ColumnType::%s);
          colmeta.fixedLen = %u;
          colmeta.fixedOffset = %u;
          colmeta.mongoType = (unsigned char)(%d);
          colmeta.mysqlType = (unsigned char)(%d);
          schema.m_columnsMeta.insert_i("%s", colmeta);
        }
)EOS", colmeta.typeNameString(), unsigned(colmeta.fixedLen)
	 , unsigned(colmeta.fixedOffset), colmeta.mongoType, colmeta.mysqlType
	 , colname
		);
	}
	printf(
R"EOS(      }
      return schema;
    }
)EOS"); // getSchema
#endif
	printf(
R"EOS(
    static bool
    checkSchema(const terark::terichdb::Schema& schema, bool checkColname = false) {
      using namespace terark;
      using namespace terark::terichdb;
      if (schema.columnNum() != %zd) {
        return false;
      }
)EOS", colnum);
	for (size_t i = 0; i < colnum; ++i) {
		const char* colname = schema.getColumnName(i).c_str();
		const auto& colmeta = schema.getColumnMeta(i);
		size_t fixlen = colmeta.fixedLen;
		printf(
R"EOS(      {
        const fstring     colname = schema.getColumnName(%zd);
        const ColumnMeta& colmeta = schema.getColumnMeta(%zd);
        if (checkColname && colname != "%s") {
          return false;
        }
        if (colmeta.type != ColumnType::%s) {
          return false;
        }
)EOS", i, i, colname, colmeta.typeNameString());

		if (colmeta.type == ColumnType::Fixed) {
		  printf(
R"EOS(        if (colmeta.fixedLen != %zd) {
          return false;
        }
      }
)EOS", fixlen);
		}
		else {
			printf("      }\n");
		}
	}
	printf(
R"EOS(      return true;
    }
)EOS"); // checkSchema

	printf("  }; // %s\n", className);
}

int usage(const char* prog) {
	fprintf(stderr, "usage: %s [ options ] terichdb-schema-file namespace table-name\n", prog);
	return 1;
}

std::string TransformColgroupName(fstring cgName0) {
	std::string cgName = cgName0.str();
	std::transform(cgName.begin(), cgName.end(), cgName.begin(),
		[](unsigned char ch) -> char {
			if (isalnum(ch) || '_' == ch)
				return ch;
			else
				return '_';
		});
	return cgName;
}

int main(int argc, char* argv[]) {
	std::vector<const char*> includes;
	for (;;) {
		int opt = getopt(argc, argv, "i:");
		switch (opt) {
		case -1:
			goto GetoptDone;
		case '?':
			return usage(argv[0]);
		case 'i':
			includes.push_back(optarg);
			break;
		}
	}
GetoptDone:
	if (argc - optind < 3) {
		return usage(argv[0]);
	}
	const char* jsonFilePath = argv[optind + 0];
	const char* ns = argv[optind + 1];
	const char* tabName = argv[optind + 2];
	SchemaConfig sconf;
	try {
		sconf.loadJsonFile(jsonFilePath);
	}
	catch (const std::exception& ex) {
		fprintf(stderr, "ERROR: loadJsonFile(%s) failed: %s\n", jsonFilePath, ex.what());
		return 1;
	}
	printf(R"EOS(#pragma once

//*******************************************************************
// WARNING: DO NOT modify this file!
// This file is generated by %s
// with dbmeta file: "%s"
//
// Keep "/path/to/your/database/dbmeta.json" being consitent with
// this dbmeta file: "%s"
//*******************************************************************

#include <terark/terichdb/db_table.hpp>
#include <terark/io/DataIO.hpp>
#include <terark/io/MemStream.hpp>
#include <terark/io/RangeStream.hpp>
#include <boost/static_assert.hpp>

)EOS", argv[0], jsonFilePath, jsonFilePath);
	for (const char* inc : includes) {
		printf("#include \"%s\"\n", inc);
	}
	printf("\n");
	printf("namespace %s {\n", ns);
	compileOneSchema(*sconf.m_rowSchema, tabName, tabName);
	size_t cgNum = sconf.getColgroupNum();
	for (size_t i = 0; i < cgNum; ++i) {
		const Schema& schema = *sconf.m_colgroupSchemaSet->getSchema(i);
		std::string cgName = TransformColgroupName(schema.m_name);
		std::string className = tabName;
		className += "_Colgroup_";
		className += cgName;
		compileOneSchema(schema, className.c_str(), tabName);
		if (i < sconf.m_indexSchemaSet->indexNum()) {
			printf("  typedef %s %s_Index_%s;\n\n", className.c_str(), tabName, cgName.c_str());
		}
	}

	printf(
R"EOS(
  // DbTablePtr use none-const ref is just for ensure application code:
  // var 'tab' must be a 'DbTablePtr', can not be a 'DbTable*'
  inline bool
  %s::checkTableSchema(terark::terichdb::DbTablePtr& tab, bool checkColname) {
    using namespace terark::terichdb;
    assert(tab.get() != nullptr);
    const SchemaConfig& sconf = tab->getSchemaConfig();
    if (!%s::checkSchema(*sconf.m_rowSchema)) {
      return false;
    }
)EOS", tabName, tabName);

	for (size_t i = 0; i < cgNum; ++i) {
		const Schema& schema = *sconf.m_colgroupSchemaSet->getSchema(i);
		std::string cgName = TransformColgroupName(schema.m_name);
		printf(
R"EOS(    if (!%s_Colgroup_%s::checkSchema(
          sconf.getColgroupSchema(%zd), checkColname)) {
      return false;
    }
)EOS", tabName, cgName.c_str(), i);
	}

	printf(
R"EOS(    return true;
  } // %s

} // namespace %s
)EOS", tabName, ns);

    return 0;
}

