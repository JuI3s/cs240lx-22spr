; ModuleID = './objs/test-no-error.ll'
source_filename = "test-no-error.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx13.0.0"

@.str = private unnamed_addr constant [39 x i8] c"Should have run without any errors...\0A\00", align 1

; Function Attrs: noinline nounwind ssp uwtable
define void @test() #0 {
  call void @log_stack(i8* inttoptr (i32 1 to i8*))
  %1 = alloca i8*, align 8
  call void @log_stack(i8* inttoptr (i32 1 to i8*))
  %2 = alloca i8, align 1
  %3 = call i8* @malloc(i64 noundef 8) #3
  call void @log_malloc(i8* %3, i64 8)
  %4 = bitcast i8** %1 to i8*
  call void @log_store(i8* %4)
  store i8* %3, i8** %1, align 8
  %5 = bitcast i8** %1 to i8*
  call void @log_load(i8* %5)
  %6 = load i8*, i8** %1, align 8
  %7 = getelementptr inbounds i8, i8* %6, i64 1
  call void @log_store(i8* %7)
  store i8 8, i8* %7, align 1
  %8 = bitcast i8** %1 to i8*
  call void @log_load(i8* %8)
  %9 = load i8*, i8** %1, align 8
  %10 = getelementptr inbounds i8, i8* %9, i64 1
  call void @log_load(i8* %10)
  %11 = load i8, i8* %10, align 1
  call void @log_store(i8* %2)
  store i8 %11, i8* %2, align 1
  %12 = bitcast i8** %1 to i8*
  call void @log_load(i8* %12)
  %13 = load i8*, i8** %1, align 8
  call void @free(i8* noundef %13)
  call void @log_free(i8* %13)
  %14 = call i8* @malloc(i64 noundef 8) #3
  call void @log_malloc(i8* %14, i64 8)
  %15 = bitcast i8** %1 to i8*
  call void @log_store(i8* %15)
  store i8* %14, i8** %1, align 8
  %16 = bitcast i8** %1 to i8*
  call void @log_load(i8* %16)
  %17 = load i8*, i8** %1, align 8
  call void @free(i8* noundef %17)
  call void @log_free(i8* %17)
  %18 = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([39 x i8], [39 x i8]* @.str, i64 0, i64 0))
  ret void
}

; Function Attrs: allocsize(0)
declare i8* @malloc(i64 noundef) #1

declare void @free(i8* noundef) #2

declare i32 @printf(i8* noundef, ...) #2

; Function Attrs: noinline nounwind ssp uwtable
define i32 @main() #0 {
  call void @init_check()
  call void @log_stack(i8* inttoptr (i32 1 to i8*))
  %1 = alloca i32, align 4
  %2 = bitcast i32* %1 to i8*
  call void @log_store(i8* %2)
  store i32 0, i32* %1, align 4
  call void @test()
  call void @exit_check()
  ret i32 0
}

declare void @print_hello()

declare void @log_malloc(i8*, i64)

declare void @log_free(i8*)

declare void @log_load(i8*)

declare void @log_store(i8*)

declare void @log_stack(i8*)

declare void @init_check()

declare void @exit_check()

attributes #0 = { noinline nounwind ssp uwtable "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+crc,+crypto,+dotprod,+fp-armv8,+fp16fml,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+v8.5a,+zcm,+zcz" }
attributes #1 = { allocsize(0) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+crc,+crypto,+dotprod,+fp-armv8,+fp16fml,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+v8.5a,+zcm,+zcz" }
attributes #2 = { "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+crc,+crypto,+dotprod,+fp-armv8,+fp16fml,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+v8.5a,+zcm,+zcz" }
attributes #3 = { allocsize(0) }

!llvm.module.flags = !{!0, !1, !2, !3, !4, !5, !6, !7}
!llvm.ident = !{!8}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, !"branch-target-enforcement", i32 0}
!2 = !{i32 1, !"sign-return-address", i32 0}
!3 = !{i32 1, !"sign-return-address-all", i32 0}
!4 = !{i32 1, !"sign-return-address-with-bkey", i32 0}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"uwtable", i32 1}
!7 = !{i32 7, !"frame-pointer", i32 1}
!8 = !{!"Homebrew clang version 14.0.6"}
