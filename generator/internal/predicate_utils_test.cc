// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "generator/internal/predicate_utils.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/text_format.h>
#include <gmock/gmock.h>

namespace google {
namespace cloud {
namespace generator_internal {
namespace {

using ::google::protobuf::DescriptorPool;
using ::google::protobuf::FieldDescriptorProto;
using ::google::protobuf::FileDescriptor;
using ::google::protobuf::FileDescriptorProto;

bool PredicateTrue(int const&) { return true; }
bool PredicateFalse(int const&) { return false; }

TEST(PredicateUtilsTest, GenericNot) {
  int const unused = 0;
  EXPECT_TRUE(GenericNot<int>(PredicateFalse)(unused));
  EXPECT_FALSE(GenericNot<int>(PredicateTrue)(unused));
}

TEST(PredicateUtilsTest, GenericAnd) {
  int const unused = 0;
  EXPECT_TRUE(GenericAnd<int>(PredicateTrue, PredicateTrue)(unused));
  EXPECT_FALSE(GenericAnd<int>(PredicateFalse, PredicateTrue)(unused));
  EXPECT_FALSE(GenericAnd<int>(PredicateTrue, PredicateFalse)(unused));
  EXPECT_FALSE(GenericAnd<int>(PredicateFalse, PredicateFalse)(unused));

  EXPECT_TRUE(
      GenericAnd<int>(PredicateTrue, GenericNot<int>(PredicateFalse))(unused));
  EXPECT_TRUE(
      GenericAnd<int>(GenericNot<int>(PredicateFalse), PredicateTrue)(unused));
  EXPECT_TRUE(
      GenericNot<int>(GenericAnd<int>(PredicateTrue, PredicateFalse))(unused));
}

TEST(PredicateUtilsTest, GenericOr) {
  int const unused = 0;
  EXPECT_TRUE(GenericOr<int>(PredicateTrue, PredicateTrue)(unused));
  EXPECT_TRUE(GenericOr<int>(PredicateFalse, PredicateTrue)(unused));
  EXPECT_TRUE(GenericOr<int>(PredicateTrue, PredicateFalse)(unused));
  EXPECT_FALSE(GenericOr<int>(PredicateFalse, PredicateFalse)(unused));
}

TEST(PredicateUtilsTest, GenericAll) {
  int const unused = 0;
  EXPECT_TRUE(GenericAll<int>(PredicateTrue)(unused));
  EXPECT_FALSE(GenericAll<int>(PredicateFalse)(unused));
  EXPECT_FALSE(GenericAll<int>(PredicateFalse, PredicateFalse)(unused));
  EXPECT_TRUE(GenericAll<int>(PredicateTrue, PredicateTrue)(unused));
  EXPECT_TRUE(
      GenericAll<int>(PredicateTrue, PredicateTrue, PredicateTrue)(unused));
  EXPECT_FALSE(
      GenericAll<int>(PredicateFalse, PredicateTrue, PredicateTrue)(unused));
  EXPECT_FALSE(
      GenericAll<int>(PredicateTrue, PredicateFalse, PredicateTrue)(unused));
  EXPECT_FALSE(
      GenericAll<int>(PredicateFalse, PredicateFalse, PredicateFalse)(unused));

  EXPECT_FALSE(GenericAll<int>(
      PredicateFalse, GenericOr<int>(PredicateFalse, PredicateTrue))(unused));
}

TEST(PredicateUtilsTest, GenericAny) {
  int const unused = 0;
  EXPECT_TRUE(GenericAny<int>(PredicateTrue)(unused));
  EXPECT_FALSE(GenericAny<int>(PredicateFalse)(unused));
  EXPECT_FALSE(GenericAny<int>(PredicateFalse, PredicateFalse)(unused));
  EXPECT_TRUE(GenericAny<int>(PredicateTrue, PredicateTrue)(unused));
  EXPECT_TRUE(
      GenericAny<int>(PredicateTrue, PredicateTrue, PredicateTrue)(unused));
  EXPECT_TRUE(
      GenericAny<int>(PredicateFalse, PredicateTrue, PredicateTrue)(unused));
  EXPECT_TRUE(
      GenericAny<int>(PredicateTrue, PredicateFalse, PredicateTrue)(unused));
  EXPECT_FALSE(
      GenericAny<int>(PredicateFalse, PredicateFalse, PredicateFalse)(unused));
}

TEST(PredicateUtilsTest, IsResponseTypeEmpty) {
  FileDescriptorProto service_file;
  service_file.set_name("google/foo/v1/service.proto");
  service_file.add_service()->set_name("Service");
  *service_file.mutable_package() = "google.protobuf";
  auto* input_message = service_file.add_message_type();
  input_message->set_name("Bar");
  auto* output_message = service_file.add_message_type();
  output_message->set_name("Empty");

  auto* empty_method = service_file.mutable_service(0)->add_method();
  *empty_method->mutable_name() = "Empty";
  *empty_method->mutable_input_type() = "google.protobuf.Bar";
  *empty_method->mutable_output_type() = "google.protobuf.Empty";

  auto* non_empty_method = service_file.mutable_service(0)->add_method();
  *non_empty_method->mutable_name() = "NonEmpty";
  *non_empty_method->mutable_input_type() = "google.protobuf.Bar";
  *non_empty_method->mutable_output_type() = "google.protobuf.Bar";

  DescriptorPool pool;
  FileDescriptor const* service_file_descriptor = pool.BuildFile(service_file);
  EXPECT_TRUE(
      IsResponseTypeEmpty(*service_file_descriptor->service(0)->method(0)));
  EXPECT_FALSE(
      IsResponseTypeEmpty(*service_file_descriptor->service(0)->method(1)));
}

TEST(PredicateUtilsTest, IsLongrunningOperation) {
  FileDescriptorProto service_file;
  service_file.set_name("google/foo/v1/service.proto");
  service_file.add_service()->set_name("Service");
  *service_file.mutable_package() = "google.longrunning";
  auto* input_message = service_file.add_message_type();
  input_message->set_name("Bar");
  auto* output_message = service_file.add_message_type();
  output_message->set_name("Operation");

  auto* lro_method = service_file.mutable_service(0)->add_method();
  *lro_method->mutable_name() = "Lro";
  *lro_method->mutable_input_type() = "google.longrunning.Bar";
  *lro_method->mutable_output_type() = "google.longrunning.Operation";

  auto* not_lro_method = service_file.mutable_service(0)->add_method();
  *not_lro_method->mutable_name() = "NonLro";
  *not_lro_method->mutable_input_type() = "google.longrunning.Bar";
  *not_lro_method->mutable_output_type() = "google.longrunning.Bar";

  DescriptorPool pool;
  FileDescriptor const* service_file_descriptor = pool.BuildFile(service_file);
  EXPECT_TRUE(
      IsLongrunningOperation(*service_file_descriptor->service(0)->method(0)));
  EXPECT_FALSE(
      IsLongrunningOperation(*service_file_descriptor->service(0)->method(1)));
}

TEST(PredicateUtilsTest, IsNonStreaming) {
  FileDescriptorProto service_file;
  service_file.set_name("google/foo/v1/service.proto");
  service_file.add_service()->set_name("Service");
  *service_file.mutable_package() = "google.protobuf";
  auto* input_message = service_file.add_message_type();
  input_message->set_name("Input");
  auto* output_message = service_file.add_message_type();
  output_message->set_name("Output");

  auto* non_streaming = service_file.mutable_service(0)->add_method();
  *non_streaming->mutable_name() = "NonStreaming";
  *non_streaming->mutable_input_type() = "google.protobuf.Input";
  *non_streaming->mutable_output_type() = "google.protobuf.Output";

  auto* client_streaming = service_file.mutable_service(0)->add_method();
  *client_streaming->mutable_name() = "ClientStreaming";
  *client_streaming->mutable_input_type() = "google.protobuf.Input";
  *client_streaming->mutable_output_type() = "google.protobuf.Output";
  client_streaming->set_client_streaming(true);

  auto* server_streaming = service_file.mutable_service(0)->add_method();
  *server_streaming->mutable_name() = "ServerStreaming";
  *server_streaming->mutable_input_type() = "google.protobuf.Input";
  *server_streaming->mutable_output_type() = "google.protobuf.Output";
  server_streaming->set_server_streaming(true);

  auto* bidirectional_streaming = service_file.mutable_service(0)->add_method();
  *bidirectional_streaming->mutable_name() = "BidirectionalStreaming";
  *bidirectional_streaming->mutable_input_type() = "google.protobuf.Input";
  *bidirectional_streaming->mutable_output_type() = "google.protobuf.Output";
  bidirectional_streaming->set_client_streaming(true);
  bidirectional_streaming->set_server_streaming(true);

  DescriptorPool pool;
  FileDescriptor const* service_file_descriptor_lro =
      pool.BuildFile(service_file);
  EXPECT_TRUE(
      IsNonStreaming(*service_file_descriptor_lro->service(0)->method(0)));
  EXPECT_FALSE(
      IsNonStreaming(*service_file_descriptor_lro->service(0)->method(1)));
  EXPECT_FALSE(
      IsNonStreaming(*service_file_descriptor_lro->service(0)->method(2)));
  EXPECT_FALSE(
      IsNonStreaming(*service_file_descriptor_lro->service(0)->method(3)));
}

TEST(PredicateUtilsTest, IsLongrunningMetadataTypeUsedAsResponseEmptyResponse) {
  FileDescriptorProto longrunning_file;
  auto constexpr kLongrunningText = R"pb(
    name: "google/longrunning/operation.proto"
    package: "google.longrunning"
    message_type { name: "Operation" }
  )pb";
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(kLongrunningText,
                                                            &longrunning_file));
  google::protobuf::FileDescriptorProto service_file;
  /// @cond
  auto constexpr kServiceText = R"pb(
    name: "google/foo/v1/service.proto"
    package: "google.protobuf"
    dependency: "google/longrunning/operation.proto"
    message_type { name: "Bar" }
    message_type { name: "Empty" }
    service {
      name: "Service"
      method {
        name: "Method0"
        input_type: "google.protobuf.Bar"
        output_type: "google.longrunning.Operation"
        options {
          [google.longrunning.operation_info] {
            response_type: "google.protobuf.Empty"
            metadata_type: "google.protobuf.Method2Metadata"
          }
          [google.api.http] {
            put: "/v1/{parent=projects/*/instances/*}/databases"
          }
        }
      }
    }
  )pb";
  /// @endcond
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(kServiceText,
                                                            &service_file));
  DescriptorPool pool;
  pool.BuildFile(longrunning_file);
  FileDescriptor const* service_file_descriptor = pool.BuildFile(service_file);
  EXPECT_TRUE(IsLongrunningMetadataTypeUsedAsResponse(
      *service_file_descriptor->service(0)->method(0)));
}

TEST(PredicateUtilsTest,
     IsLongrunningMetadataTypeUsedAsResponseNonEmptyResponse) {
  FileDescriptorProto longrunning_file;
  auto constexpr kLongrunningText = R"pb(
    name: "google/longrunning/operation.proto"
    package: "google.longrunning"
    message_type { name: "Operation" }
  )pb";
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(kLongrunningText,
                                                            &longrunning_file));
  google::protobuf::FileDescriptorProto service_file;
  /// @cond
  auto constexpr kServiceText = R"pb(
    name: "google/foo/v1/service.proto"
    package: "google.protobuf"
    dependency: "google/longrunning/operation.proto"
    message_type { name: "Bar" }
    message_type { name: "Empty" }
    service {
      name: "Service"
      method {
        name: "Method0"
        input_type: "google.protobuf.Bar"
        output_type: "google.longrunning.Operation"
        options {
          [google.longrunning.operation_info] {
            response_type: "google.protobuf.Method2Response"
            metadata_type: "google.protobuf.Method2Metadata"
          }
          [google.api.http] {
            patch: "/v1/{parent=projects/*/instances/*}/databases"
          }
        }
      }
    }
  )pb";
  /// @endcond
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(kServiceText,
                                                            &service_file));
  DescriptorPool pool;
  pool.BuildFile(longrunning_file);
  FileDescriptor const* service_file_descriptor = pool.BuildFile(service_file);
  EXPECT_FALSE(IsLongrunningMetadataTypeUsedAsResponse(
      *service_file_descriptor->service(0)->method(0)));
}

TEST(PredicateUtilsTest,
     IsLongrunningMetadataTypeUsedAsResponseNotLongrunning) {
  google::protobuf::FileDescriptorProto service_file;
  /// @cond
  auto constexpr kServiceText = R"pb(
    name: "google/foo/v1/service.proto"
    package: "google.protobuf"
    message_type { name: "Bar" }
    message_type { name: "Empty" }
    service {
      name: "Service"
      method {
        name: "Method0"
        input_type: "google.protobuf.Bar"
        output_type: "google.protobuf.Empty"
        options {
          [google.api.http] {
            patch: "/v1/{parent=projects/*/instances/*}/databases"
          }
        }
      }
    }
  )pb";
  /// @endcond
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(kServiceText,
                                                            &service_file));
  DescriptorPool pool;
  FileDescriptor const* service_file_descriptor = pool.BuildFile(service_file);
  EXPECT_FALSE(IsLongrunningMetadataTypeUsedAsResponse(
      *service_file_descriptor->service(0)->method(0)));
}

TEST(PredicateUtilsTest, PaginationSuccess) {
  FileDescriptorProto service_file;
  service_file.set_name("google/foo/v1/service.proto");
  service_file.add_service()->set_name("Service");
  *service_file.mutable_package() = "google.protobuf";
  auto* repeated_message = service_file.add_message_type();
  repeated_message->set_name("Bar");

  auto* input_message = service_file.add_message_type();
  input_message->set_name("Input");
  auto* page_size_field = input_message->add_field();
  page_size_field->set_name("page_size");
  page_size_field->set_type(protobuf::FieldDescriptorProto_Type_TYPE_INT32);
  page_size_field->set_number(1);

  auto* page_token_field = input_message->add_field();
  page_token_field->set_name("page_token");
  page_token_field->set_type(protobuf::FieldDescriptorProto_Type_TYPE_STRING);
  page_token_field->set_number(2);

  auto* output_message = service_file.add_message_type();
  output_message->set_name("Output");
  auto* next_page_token_field = output_message->add_field();
  next_page_token_field->set_name("next_page_token");
  next_page_token_field->set_type(
      protobuf::FieldDescriptorProto_Type_TYPE_STRING);
  next_page_token_field->set_number(1);

  FieldDescriptorProto* repeated_message_field = output_message->add_field();
  repeated_message_field->set_name("repeated_field");
  repeated_message_field->set_type(
      protobuf::FieldDescriptorProto_Type_TYPE_MESSAGE);
  repeated_message_field->set_label(
      protobuf::FieldDescriptorProto_Label_LABEL_REPEATED);
  repeated_message_field->set_type_name("google.protobuf.Bar");
  repeated_message_field->set_number(2);

  auto* paginated_method = service_file.mutable_service(0)->add_method();
  *paginated_method->mutable_name() = "Paginated";
  *paginated_method->mutable_input_type() = "google.protobuf.Input";
  *paginated_method->mutable_output_type() = "google.protobuf.Output";

  DescriptorPool pool;
  FileDescriptor const* service_file_descriptor = pool.BuildFile(service_file);
  EXPECT_TRUE(IsPaginated(*service_file_descriptor->service(0)->method(0)));
  auto result =
      DeterminePagination(*service_file_descriptor->service(0)->method(0));
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result->first, "repeated_field");
  EXPECT_EQ(result->second, "google.protobuf.Bar");
}

TEST(PredicateUtilsTest, PaginationNoPageSize) {
  FileDescriptorProto service_file;
  service_file.set_name("google/foo/v1/service.proto");
  service_file.add_service()->set_name("Service");
  *service_file.mutable_package() = "google.protobuf";
  auto* repeated_message = service_file.add_message_type();
  repeated_message->set_name("Bar");

  auto* input_message = service_file.add_message_type();
  input_message->set_name("Input");

  auto* output_message = service_file.add_message_type();
  output_message->set_name("Output");

  auto* no_page_size_method = service_file.mutable_service(0)->add_method();
  *no_page_size_method->mutable_name() = "NoPageSize";
  *no_page_size_method->mutable_input_type() = "google.protobuf.Input";
  *no_page_size_method->mutable_output_type() = "google.protobuf.Output";

  DescriptorPool pool;
  FileDescriptor const* service_file_descriptor = pool.BuildFile(service_file);
  EXPECT_FALSE(IsPaginated(*service_file_descriptor->service(0)->method(0)));
}

TEST(PredicateUtilsTest, PaginationNoPageToken) {
  FileDescriptorProto service_file;
  service_file.set_name("google/foo/v1/service.proto");
  service_file.add_service()->set_name("Service");
  *service_file.mutable_package() = "google.protobuf";
  auto* repeated_message = service_file.add_message_type();
  repeated_message->set_name("Bar");

  auto* input_message = service_file.add_message_type();
  input_message->set_name("Input");
  auto* page_size_field = input_message->add_field();
  page_size_field->set_name("page_size");
  page_size_field->set_type(protobuf::FieldDescriptorProto_Type_TYPE_INT32);
  page_size_field->set_number(1);

  auto* output_message = service_file.add_message_type();
  output_message->set_name("Output");

  auto* no_page_token_method = service_file.mutable_service(0)->add_method();
  *no_page_token_method->mutable_name() = "NoPageToken";
  *no_page_token_method->mutable_input_type() = "google.protobuf.Input";
  *no_page_token_method->mutable_output_type() = "google.protobuf.Output";

  DescriptorPool pool;
  FileDescriptor const* service_file_descriptor = pool.BuildFile(service_file);
  EXPECT_FALSE(IsPaginated(*service_file_descriptor->service(0)->method(0)));
}

TEST(PredicateUtilsTest, PaginationNoNextPageToken) {
  FileDescriptorProto service_file;
  service_file.set_name("google/foo/v1/service.proto");
  service_file.add_service()->set_name("Service");
  *service_file.mutable_package() = "google.protobuf";
  auto* repeated_message = service_file.add_message_type();
  repeated_message->set_name("Bar");

  auto* input_message = service_file.add_message_type();
  input_message->set_name("Input");
  auto* page_size_field = input_message->add_field();
  page_size_field->set_name("page_size");
  page_size_field->set_type(protobuf::FieldDescriptorProto_Type_TYPE_INT32);
  page_size_field->set_number(1);

  auto* page_token_field = input_message->add_field();
  page_token_field->set_name("page_token");
  page_token_field->set_type(protobuf::FieldDescriptorProto_Type_TYPE_STRING);
  page_token_field->set_number(2);

  auto* output_message = service_file.add_message_type();
  output_message->set_name("Output");

  auto* no_next_page_token_method =
      service_file.mutable_service(0)->add_method();
  *no_next_page_token_method->mutable_name() = "NoNextPageToken";
  *no_next_page_token_method->mutable_input_type() = "google.protobuf.Input";
  *no_next_page_token_method->mutable_output_type() = "google.protobuf.Output";

  DescriptorPool pool;
  FileDescriptor const* service_file_descriptor = pool.BuildFile(service_file);
  EXPECT_FALSE(IsPaginated(*service_file_descriptor->service(0)->method(0)));
}

TEST(PredicateUtilsTest, PaginationNoRepeatedMessageField) {
  FileDescriptorProto service_file;
  service_file.set_name("google/foo/v1/service.proto");
  service_file.add_service()->set_name("Service");
  *service_file.mutable_package() = "google.protobuf";
  auto* repeated_message = service_file.add_message_type();
  repeated_message->set_name("Bar");

  auto* input_message = service_file.add_message_type();
  input_message->set_name("Input");
  auto* page_size_field = input_message->add_field();
  page_size_field->set_name("page_size");
  page_size_field->set_type(protobuf::FieldDescriptorProto_Type_TYPE_INT32);
  page_size_field->set_number(1);

  auto* page_token_field = input_message->add_field();
  page_token_field->set_name("page_token");
  page_token_field->set_type(protobuf::FieldDescriptorProto_Type_TYPE_STRING);
  page_token_field->set_number(2);

  auto* output_message = service_file.add_message_type();
  output_message->set_name("Output");
  auto* next_page_token_field = output_message->add_field();
  next_page_token_field->set_name("next_page_token");
  next_page_token_field->set_type(
      protobuf::FieldDescriptorProto_Type_TYPE_STRING);
  next_page_token_field->set_number(1);

  FieldDescriptorProto* repeated_int32_field = output_message->add_field();
  repeated_int32_field->set_name("repeated_field");
  repeated_int32_field->set_type(
      protobuf::FieldDescriptorProto_Type_TYPE_INT32);
  repeated_int32_field->set_label(
      protobuf::FieldDescriptorProto_Label_LABEL_REPEATED);
  repeated_int32_field->set_number(2);

  auto* no_repeated_message_method =
      service_file.mutable_service(0)->add_method();
  *no_repeated_message_method->mutable_name() = "NoRepeatedMessage";
  *no_repeated_message_method->mutable_input_type() = "google.protobuf.Input";
  *no_repeated_message_method->mutable_output_type() = "google.protobuf.Output";

  DescriptorPool pool;
  FileDescriptor const* service_file_descriptor = pool.BuildFile(service_file);
  EXPECT_FALSE(IsPaginated(*service_file_descriptor->service(0)->method(0)));
}

TEST(PredicateUtilsDeathTest, PaginationRepeatedMessageOrderMismatch) {
  FileDescriptorProto service_file;
  service_file.set_name("google/foo/v1/service.proto");
  service_file.add_service()->set_name("Service");
  *service_file.mutable_package() = "google.protobuf";
  auto* repeated_message = service_file.add_message_type();
  repeated_message->set_name("Bar");
  auto* repeated_message2 = service_file.add_message_type();
  repeated_message2->set_name("Foo");

  auto* input_message = service_file.add_message_type();
  input_message->set_name("Input");
  auto* page_size_field = input_message->add_field();
  page_size_field->set_name("page_size");
  page_size_field->set_type(protobuf::FieldDescriptorProto_Type_TYPE_INT32);
  page_size_field->set_number(1);

  FieldDescriptorProto* page_token_field;
  page_token_field = input_message->add_field();
  page_token_field->set_name("page_token");
  page_token_field->set_type(protobuf::FieldDescriptorProto_Type_TYPE_STRING);
  page_token_field->set_number(2);

  auto* output_message = service_file.add_message_type();
  output_message->set_name("Output");
  auto* next_page_token_field = output_message->add_field();
  next_page_token_field->set_name("next_page_token");
  next_page_token_field->set_type(
      protobuf::FieldDescriptorProto_Type_TYPE_STRING);
  next_page_token_field->set_number(1);

  FieldDescriptorProto* repeated_message_field_first =
      output_message->add_field();
  repeated_message_field_first->set_name("repeated_message_field_first");
  repeated_message_field_first->set_type(
      protobuf::FieldDescriptorProto_Type_TYPE_MESSAGE);
  repeated_message_field_first->set_label(
      protobuf::FieldDescriptorProto_Label_LABEL_REPEATED);
  repeated_message_field_first->set_type_name("google.protobuf.Foo");
  repeated_message_field_first->set_number(3);

  FieldDescriptorProto* repeated_message_field_second =
      output_message->add_field();
  repeated_message_field_second->set_name("repeated_message_field_second");
  repeated_message_field_second->set_type(
      protobuf::FieldDescriptorProto_Type_TYPE_MESSAGE);
  repeated_message_field_second->set_label(
      protobuf::FieldDescriptorProto_Label_LABEL_REPEATED);
  repeated_message_field_second->set_type_name("google.protobuf.Bar");
  repeated_message_field_second->set_number(2);

  auto* repeated_order_mismatch_method =
      service_file.mutable_service(0)->add_method();
  *repeated_order_mismatch_method->mutable_name() = "RepeatedOrderMismatch";
  *repeated_order_mismatch_method->mutable_input_type() =
      "google.protobuf.Input";
  *repeated_order_mismatch_method->mutable_output_type() =
      "google.protobuf.Output";

  DescriptorPool pool;
  FileDescriptor const* service_file_descriptor = pool.BuildFile(service_file);
  EXPECT_DEATH(IsPaginated(*service_file_descriptor->service(0)->method(0)),
               "");
}

TEST(PredicateUtilsTest, PredicatedFragmentTrueString) {
  int const unused = 0;
  PredicatedFragment<int> f = {PredicateTrue, "True", "False"};
  EXPECT_EQ(f(unused), "True");
}

TEST(PredicateUtilsTest, PredicatedFragmentFalseString) {
  int const unused = 0;
  PredicatedFragment<int> f = {PredicateFalse, "True", "False"};
  EXPECT_EQ(f(unused), "False");
}

TEST(PredicateUtilsTest, PredicatedFragmentStringOnly) {
  int const unused = 0;
  PredicatedFragment<int> f = {"True"};
  EXPECT_EQ(f(unused), "True");
}

TEST(Pattern, OperatorParens) {
  int const unused = 0;
  Pattern<int> p({}, PredicateFalse);
  EXPECT_FALSE(p(unused));
}

TEST(Pattern, FragmentsAccessor) {
  int const unused = 0;
  Pattern<int> p({{PredicateFalse, "fragment0_true", "fragment0_false"},
                  {PredicateTrue, "fragment1_true", "fragment1_false"}},
                 PredicateTrue);
  EXPECT_TRUE(p(unused));
  std::string result;
  for (auto const& pf : p.fragments()) {
    result += pf(unused);
  }

  EXPECT_EQ(result, std::string("fragment0_falsefragment1_true"));
}

}  // namespace
}  // namespace generator_internal
}  // namespace cloud
}  // namespace google
