@echo off
setlocal

echo Adding modified files to git staging...
git add src/OpenAI_API_Client.cpp
git add src/OpenAI_API_Client.h
git add CMakeLists.txt
git add tests/AIAgentOrchestratorTest.cpp
git add build_with_cpr.bat
git add test_openai_client_with_cpr.bat
git add docs/CPR_IMPLEMENTATION.md
git add CPR_VS_CURL_FINDINGS.md
git add DIRECT_CURL_APPROACH.md
git add test_curl.bat
git add test_openai_client.bat
git add README.md
git add src/curl_test.cpp
git add curl_test_cmake/CMakeLists.txt

echo Displaying status of staged files:
git status

echo.
echo Run the following command to commit (after reviewing):
echo git commit -F COMMIT_MESSAGE.txt
echo.
echo Or to edit the commit message before committing:
echo git commit -e -F COMMIT_MESSAGE.txt 