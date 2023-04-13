// Copyright 2020-present Open Networking Foundation
// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/tdi_lut_manager.h"

#include <string>
#include <vector>

#include "absl/memory/memory.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "gtest/gtest-message.h"
#include "stratum/glue/status/status_test_util.h"
#include "stratum/hal/lib/tdi/tdi_sde_mock.h"
#include "stratum/lib/utils.h"

namespace stratum {
namespace hal {
namespace tdi {

using ::testing::_;
using ::testing::DoAll;
using ::testing::ByMove;
using ::testing::HasSubstr;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;

MATCHER_P(DerivedFromStatus, status, "") {
    if (arg.error_code() != status.error_code()) {
        return false;
    }
    if (arg.error_message().find(status.error_message()) == std::string::npos) {
        *result_listener << "\nOriginal error string: \"" << status.error_message()
        << "\" is missing from the actual status.";
        return false;
    }
    return true;
}

constexpr char kUnspExternMsg[] = "Unsupported extern type 0";
constexpr char kUnspReadOpMsg[] = "READ VLUT ENTRY IS YET TO BE SUPPORTED";
constexpr uint32 kMvlutExactMatch = 129;
constexpr uint32 kMvlutTernaryMatch = 130;
constexpr int kP4TableId = 1;
constexpr int kTdiRtTableId = 1;

class TdiLutManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        tdi_sde_wrapper_mock_ = absl::make_unique<TdiSdeMock>();
        tdi_lut_manager_ = TdiLutManager::CreateInstance(
            OPERATION_MODE_STANDALONE, tdi_sde_wrapper_mock_.get(), kDevice1);
    }

    ::util::Status InvalidParamError() {
        return ::util::Status(StratumErrorSpace(), ERR_INVALID_PARAM, kUnspExternMsg);
    }

    ::util::Status InternalError() {
        return ::util::Status(StratumErrorSpace(), ERR_INTERNAL, kUnspReadOpMsg);
    }

    static constexpr int kDevice1 = 0;

    std::unique_ptr<TdiSdeMock> tdi_sde_wrapper_mock_;
    std::unique_ptr<TdiLutManager> tdi_lut_manager_;
};

TEST_F(TdiLutManagerTest, ReadTableEntryTestExactMatch) {
    auto session_mock = std::make_shared<SessionMock>();
    WriterInterface<::p4::v1::ReadResponse>* writer;
    ::p4::v1::ExternEntry extern_entry;

    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    auto* param = new ::p4::v1::MvlutParam();
    auto* any = new google::protobuf::Any();

    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");

    mvlut_entry->set_mvlut_id(1);
    mvlut_entry->add_match()->set_field_id(1);
    mvlut_entry->set_allocated_param(param);

    extern_entry.set_extern_type_id(kMvlutExactMatch);
    extern_entry.set_extern_id(1);
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    EXPECT_CALL(*tdi_sde_wrapper_mock_, GetTdiRtId(kP4TableId))
        .WillOnce(Return(kTdiRtTableId));

    EXPECT_THAT(tdi_lut_manager_->ReadTableEntry(session_mock, extern_entry,
        writer), DerivedFromStatus(InternalError()));
}

TEST_F(TdiLutManagerTest, ReadTableEntryTestTernaryMatch) {
    auto session_mock = std::make_shared<SessionMock>();
    WriterInterface<::p4::v1::ReadResponse>* writer;
    ::p4::v1::ExternEntry extern_entry;

    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    auto* param = new ::p4::v1::MvlutParam();
    auto* any = new google::protobuf::Any();

    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");

    mvlut_entry->set_mvlut_id(1);
    mvlut_entry->add_match()->set_field_id(1);
    mvlut_entry->set_allocated_param(param);

    extern_entry.set_extern_type_id(kMvlutTernaryMatch);
    extern_entry.set_extern_id(1);
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    EXPECT_CALL(*tdi_sde_wrapper_mock_, GetTdiRtId(kP4TableId))
        .WillOnce(Return(kTdiRtTableId));

    EXPECT_THAT(tdi_lut_manager_->ReadTableEntry(session_mock, extern_entry,
        writer), DerivedFromStatus(InternalError()));
}

TEST_F(TdiLutManagerTest, ReadTableEntryTestUnsupportedType) {
    auto session_mock = std::make_shared<SessionMock>();
    WriterInterface<::p4::v1::ReadResponse>* writer;
    ::p4::v1::ExternEntry entry;

    EXPECT_THAT(tdi_lut_manager_->ReadTableEntry(session_mock, entry,
        writer), DerivedFromStatus(InvalidParamError()));
}

TEST_F(TdiLutManagerTest, WriteTableEntryTestExactMatchInsert) {
    auto session_mock = std::make_shared<SessionMock>();
    ::p4::v1::ExternEntry extern_entry;

    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    auto* param = new ::p4::v1::MvlutParam();
    auto* any = new google::protobuf::Any();
    auto* exact = new ::p4::v1::FieldMatch_Exact();

    auto table_key_mock = absl::make_unique<TableKeyMock>();
    auto table_data_mock = absl::make_unique<TableDataMock>();

    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");

    exact->set_value("1");

    mvlut_entry->set_mvlut_id(1);
    auto match_field = mvlut_entry->add_match();
    match_field->set_field_id(1);
    match_field->set_allocated_exact(exact);
    mvlut_entry->set_allocated_param(param);

    extern_entry.set_extern_type_id(kMvlutExactMatch);
    extern_entry.set_extern_id(1);
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    EXPECT_CALL(*tdi_sde_wrapper_mock_, InsertTableEntry(kDevice1, _,
        kTdiRtTableId, table_key_mock.get(), table_data_mock.get()))
        .WillOnce(Return(::util::OkStatus()));

    EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableKey(kTdiRtTableId))
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableKeyInterface>>(
                std::move(table_key_mock)))));

    EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableData(kTdiRtTableId, _))
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableDataInterface>>(
                std::move(table_data_mock)))));

    EXPECT_OK(tdi_lut_manager_->WriteTableEntry(session_mock,
        ::p4::v1::Update::INSERT, extern_entry));
}

TEST_F(TdiLutManagerTest, WriteTableEntryTestExactMatchModify) {
    auto session_mock = std::make_shared<SessionMock>();
    ::p4::v1::ExternEntry extern_entry;

    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    auto* param = new ::p4::v1::MvlutParam();
    auto* any = new google::protobuf::Any();
    auto* exact = new ::p4::v1::FieldMatch_Exact();

    auto table_key_mock = absl::make_unique<TableKeyMock>();
    auto table_data_mock = absl::make_unique<TableDataMock>();

    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");

    exact->set_value("1");

    mvlut_entry->set_mvlut_id(1);
    auto match_field = mvlut_entry->add_match();
    match_field->set_field_id(1);
    match_field->set_allocated_exact(exact);
    mvlut_entry->set_allocated_param(param);

    extern_entry.set_extern_type_id(kMvlutExactMatch);
    extern_entry.set_extern_id(1);
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    EXPECT_CALL(*tdi_sde_wrapper_mock_, ModifyTableEntry(kDevice1, _,
        kTdiRtTableId, table_key_mock.get(), table_data_mock.get()))
        .WillOnce(Return(::util::OkStatus()));

    EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableKey(kTdiRtTableId))
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableKeyInterface>>(
                std::move(table_key_mock)))));

    EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableData(kTdiRtTableId, _))
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableDataInterface>>(
                std::move(table_data_mock)))));

    EXPECT_OK(tdi_lut_manager_->WriteTableEntry(session_mock,
        ::p4::v1::Update::MODIFY, extern_entry));
}


TEST_F(TdiLutManagerTest, WriteTableEntryTestExactMatchDelete) {
    auto session_mock = std::make_shared<SessionMock>();
    ::p4::v1::ExternEntry extern_entry;

    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    auto* param = new ::p4::v1::MvlutParam();
    auto* any = new google::protobuf::Any();
    auto* exact = new ::p4::v1::FieldMatch_Exact();

    auto table_key_mock = absl::make_unique<TableKeyMock>();
    auto table_data_mock = absl::make_unique<TableDataMock>();

    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");

    exact->set_value("1");

    mvlut_entry->set_mvlut_id(1);
    auto match_field = mvlut_entry->add_match();
    match_field->set_field_id(1);
    match_field->set_allocated_exact(exact);
    mvlut_entry->set_allocated_param(param);

    extern_entry.set_extern_type_id(kMvlutExactMatch);
    extern_entry.set_extern_id(1);
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    EXPECT_CALL(*tdi_sde_wrapper_mock_, DeleteTableEntry(kDevice1, _,
        kTdiRtTableId, table_key_mock.get()))
        .WillOnce(Return(::util::OkStatus()));

    EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableKey(kTdiRtTableId))
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableKeyInterface>>(
                std::move(table_key_mock)))));

    EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableData(kTdiRtTableId, _))
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableDataInterface>>(
                std::move(table_data_mock)))));

    EXPECT_OK(tdi_lut_manager_->WriteTableEntry(session_mock,
        ::p4::v1::Update::DELETE, extern_entry));
}

TEST_F(TdiLutManagerTest, WriteTableEntryTestTernaryMatchInsert) {
    auto session_mock = std::make_shared<SessionMock>();
    WriterInterface<::p4::v1::ReadResponse>* writer;
    ::p4::v1::ExternEntry extern_entry;

    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    auto* param = new ::p4::v1::MvlutParam();
    auto* any = new google::protobuf::Any();
    auto* ternary = new ::p4::v1::FieldMatch_Ternary();

    auto table_key_mock = absl::make_unique<TableKeyMock>();
    auto table_data_mock = absl::make_unique<TableDataMock>();

    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");

    ternary->set_value("1");
    ternary->set_mask("\xfff");

    mvlut_entry->set_mvlut_id(1);
    auto match_field = mvlut_entry->add_match();
    match_field->set_field_id(1);
    match_field->set_allocated_ternary(ternary);
    mvlut_entry->set_allocated_param(param);

    extern_entry.set_extern_type_id(kMvlutTernaryMatch);
    extern_entry.set_extern_id(1);
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    EXPECT_CALL(*tdi_sde_wrapper_mock_, InsertTableEntry(kDevice1, _,
        kTdiRtTableId, table_key_mock.get(), table_data_mock.get()))
        .WillOnce(Return(::util::OkStatus()));

    EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableKey(kTdiRtTableId))
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableKeyInterface>>(
                std::move(table_key_mock)))));

    EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableData(kTdiRtTableId, _))
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableDataInterface>>(
                std::move(table_data_mock)))));

    EXPECT_OK(tdi_lut_manager_->WriteTableEntry(session_mock,
        ::p4::v1::Update::INSERT, extern_entry));
}

TEST_F(TdiLutManagerTest, WriteTableEntryTestTernaryMatchModify) {
    auto session_mock = std::make_shared<SessionMock>();
    WriterInterface<::p4::v1::ReadResponse>* writer;
    ::p4::v1::ExternEntry extern_entry;

    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    auto* param = new ::p4::v1::MvlutParam();
    auto* any = new google::protobuf::Any();
    auto* ternary = new ::p4::v1::FieldMatch_Ternary();

    auto table_key_mock = absl::make_unique<TableKeyMock>();
    auto table_data_mock = absl::make_unique<TableDataMock>();

    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");

    ternary->set_value("1");
    ternary->set_mask("\xfff");

    mvlut_entry->set_mvlut_id(1);
    auto match_field = mvlut_entry->add_match();
    match_field->set_field_id(1);
    match_field->set_allocated_ternary(ternary);
    mvlut_entry->set_allocated_param(param);

    extern_entry.set_extern_type_id(kMvlutTernaryMatch);
    extern_entry.set_extern_id(1);
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    EXPECT_CALL(*tdi_sde_wrapper_mock_, ModifyTableEntry(kDevice1, _,
        kTdiRtTableId, table_key_mock.get(), table_data_mock.get()))
        .WillOnce(Return(::util::OkStatus()));

    EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableKey(kTdiRtTableId))
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableKeyInterface>>(
                std::move(table_key_mock)))));

    EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableData(kTdiRtTableId, _))
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableDataInterface>>(
                std::move(table_data_mock)))));

    EXPECT_OK(tdi_lut_manager_->WriteTableEntry(session_mock,
        ::p4::v1::Update::MODIFY, extern_entry));
}

TEST_F(TdiLutManagerTest, WriteTableEntryTestTernaryMatchDelete) {
    auto session_mock = std::make_shared<SessionMock>();
    WriterInterface<::p4::v1::ReadResponse>* writer;
    ::p4::v1::ExternEntry extern_entry;

    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    auto* param = new ::p4::v1::MvlutParam();
    auto* any = new google::protobuf::Any();
    auto* ternary = new ::p4::v1::FieldMatch_Ternary();

    auto table_key_mock = absl::make_unique<TableKeyMock>();
    auto table_data_mock = absl::make_unique<TableDataMock>();

    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");

    ternary->set_value("1");
    ternary->set_mask("\xfff");

    mvlut_entry->set_mvlut_id(1);
    auto match_field = mvlut_entry->add_match();
    match_field->set_field_id(1);
    match_field->set_allocated_ternary(ternary);
    mvlut_entry->set_allocated_param(param);

    extern_entry.set_extern_type_id(kMvlutTernaryMatch);
    extern_entry.set_extern_id(1);
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    EXPECT_CALL(*tdi_sde_wrapper_mock_, DeleteTableEntry(kDevice1, _,
        kTdiRtTableId, table_key_mock.get()))
        .WillOnce(Return(::util::OkStatus()));

    EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableKey(kTdiRtTableId))
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableKeyInterface>>(
                std::move(table_key_mock)))));

    EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableData(kTdiRtTableId, _))
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableDataInterface>>(
                std::move(table_data_mock)))));

    EXPECT_OK(tdi_lut_manager_->WriteTableEntry(session_mock,
        ::p4::v1::Update::DELETE, extern_entry));
}

TEST_F(TdiLutManagerTest, WriteTableEntryTestUnsupportedType) {
    auto session_mock = std::make_shared<SessionMock>();
    ::p4::v1::ExternEntry extern_entry;
    ::p4::v1::Update::Type update_type = ::p4::v1::Update::INSERT;

    EXPECT_THAT(tdi_lut_manager_->WriteTableEntry(session_mock, update_type, extern_entry),
        DerivedFromStatus(InvalidParamError()));
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
