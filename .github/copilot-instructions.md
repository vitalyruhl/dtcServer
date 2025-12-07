/*
 * GitHub Copilot Instructions for coinbase-dtc-core project
 * 
 * Communication Style:
 * - Use informal tone with "you" (not "Sie" or formal language)
 * - All code comments in English only
 * - All variable names in English only  
 * - All function names in English only
 * - All error messages in English
 * - Give only brief overview after completing tasks
 * - Provide detailed explanations only when explicitly asked
 * - NEVER use emojis in code, comments, log messages, or outputs
 * - Use plain text symbols like [SUCCESS], [ERROR], [WARNING] instead of emojis
 * 
 * Project Vision & Roadmap:
 * - PRIMARY FOCUS: Coinbase integration first (other exchanges later)
 * - MAIN PURPOSE: Bridge between SierraChart and Coinbase Advanced Trade API
 * - CORE FEATURES:
 *   1. Historical chart data retrieval (all standard timeframes)
 *   2. Local data caching/storage for performance
 *   3. Real-time DOM (Depth of Market) data streaming
 *   4. Live trading capabilities
 * - DEPLOYMENT TARGET: Docker containers with credential injection
 * - ARCHITECTURE: Modular design for easy exchange integration later
 * - SCALABILITY: Quick addition of other exchanges after Coinbase is stable
 * 
 * Current Implementation Context:
 * - This is a DTC (Data Trading Client) protocol implementation
 * - Primary exchange integration: Coinbase Pro/Advanced Trade API
 * - Uses C++17 with CMake build system
 * - Windows development environment with Visual Studio 2022
 * - Real-time market data streaming via WebSocket
 * - Financial trading application - prioritize accuracy and performance
 * 
 * Code Style Preferences:
 * - Use modern C++17 features
 * - RAII and smart pointers preferred
 * - Clear, descriptive variable names (English only)
 * - Comprehensive error handling
 * - Thread-safe implementations for concurrent operations
 * - Detailed logging for debugging (English messages only)
 * - IMMEDIATE EMOJI REPLACEMENT: If ANY emoji is detected in code, comments, or log messages, replace it immediately with plain text equivalent like [SUCCESS], [ERROR], [WARNING], [INFO]
 * - Zero tolerance for emojis in production code - they cause encoding and compatibility issues
 * 
 * Architecture Patterns:
 * - Factory pattern for exchange implementations
 * - Strategy pattern for different trading algorithms
 * - Observer pattern for market data updates
 * - Namespace organization: open_dtc_server is now the main namespace!
 * 
 * DTC Protocol Architecture (CRITICAL RULE):
 * - STRICT SEPARATION: Client NEVER communicates directly with exchange APIs
 * - ALL DATA FLOW: Client ↔ DTC Server ↔ Coinbase API
 * - Client ONLY uses DTC protocol messages (SecurityDefinitionRequest, MarketDataRequest, etc.)
 * - Server handles ALL external API communication (REST, WebSocket)
 * - NO EXCEPTIONS: Client must not import/use Coinbase REST client or any exchange-specific code
 * - Data requests: Client sends DTC message → Server fetches from Coinbase → Server sends DTC response
 * - This ensures clean protocol compliance and easy exchange swapping later
 * 
 * Namespace Documentation Requirements:
 * - MANDATORY: Every new namespace MUST be documented in /dev-info/namespaces.md
 * - BEFORE creating any new namespace, update the namespaces.md file first
 * - Include: full namespace path, purpose, location, key classes, directory mapping
 * - Follow naming conventions: lowercase with underscores, use open_dtc_server hierarchy
 * - NO EXCEPTIONS: Undocumented namespaces will break maintainability
 * - Check for conflicts with existing namespaces before creating new ones
 * - Reference /dev-info/namespaces.md when unsure about namespace structure
 * 
 * Testing Approach:
 * - Unit tests for core components
 * - Integration tests for exchange connectivity
 * - Mock implementations for testing
 * - PowerShell scripts for CI/CD automation
 * - Comprehensive DTC Test Client: Create a complete test client that validates ALL DTC protocol functions with real data
 * - All DTC protocol functions must be strictly conformant to protocol specifications
 * - Every DTC feature must have automated testing coverage in GitHub Actions
 * - Real data validation: Test client should verify actual Coinbase data integration, not mock data
 * 
 * Performance Priorities:
 * - Low-latency market data processing
 * - Efficient memory management
 * - Minimal allocation in hot paths
 * - Lock-free data structures where possible
 * 
 * File Organization Guidelines:
 * - ALL documentation and development notes go in /dev-info/ directory
 * - ALL tests (unit, integration, scripts) go in /tests/ directory
 * - Keep root directory clean - only essential project files (CMakeLists.txt, README.md, etc.)
 * - GitHub-specific files (.github/FUNDING.yml, SECURITY.md, etc.) stay in their conventional locations
 * - Move obsolete or temporary files to appropriate directories or remove them
 * 
 * Research & Idea Management:
 * - ALWAYS check the /Ideas/ directory for existing solutions and research before starting new tasks
 * - Review all contents including links to avoid redundant work if solutions already exist
 * - Reference existing ideas and implementations to build upon previous work
 * 
 * API Documentation & External Information Handling:
 * - When fetching information from the internet (APIs, documentation, etc.):
 *   1. Create a subdirectory in /dev-info/ for the specific source (e.g., /dev-info/coinbase-api/)
 *   2. Save important information in organized .md files with timestamps
 *   3. Include source URLs and fetch dates for reference
 *   4. Before fetching new information, ALWAYS check existing /dev-info/ subdirectories first
 *   5. Only fetch from internet if information is missing or outdated
 * - Example structure: /dev-info/coinbase-api/endpoints.md, /dev-info/coinbase-api/authentication.md
 * - Include version info and last-updated dates in saved documentation
 * - This prevents unnecessary re-fetching and preserves valuable research
 * 
 * TODO Management & Project Tracking:
 * - ALWAYS keep /dev-info/TODO.md updated with current project status
 * - Update TODO.md immediately after completing any major feature or milestone
 * - Mark tasks as ✅ COMPLETED when done, 🚧 CURRENT when working on them
 * - Add new priorities and features as they emerge during development
 * - Reference TODO.md to understand current priorities and completed work
 * Mock/Demo Data Policy: All simulated data MUST be clearly labeled with [MOCKED DATA] prefix
 * [MOCKED DATA] labels must be placed at the BEGINNING of log messages, not at the end
 * Never present fake data as real without explicit warning to user
 * Rule: Always prefix simulated/demo content with "[MOCKED DATA]" at the start of the message
 * CRITICAL: NO MOCK DATA WITHOUT EXPLICIT USER REQUEST - only implement mock/test data when user explicitly asks for it
 * Production code should always fetch real data from APIs - mock data only for testing when specifically requested
 * 
 * Executable Path Policy:
 * - ALWAYS use full absolute paths for executables to prevent PowerShell path confusion
 * - DTC Server executable: & "C:\Daten\_Codding\coinbase-dtc-core\build\Release\coinbase_dtc_server.exe"
 * - DTC Server with credentials: cd C:\Daten\_Codding\coinbase-dtc-core\build\Release; .\coinbase_dtc_server.exe --credentials "C:\Daten\_Codding\coinbase-dtc-core\secrets\coinbase\cdp_api_key_ECDSA.json"
 * - GUI Test Client executable: & "C:\Daten\_Codding\coinbase-dtc-core\build\Release\dtc_test_client_gui.exe"
 * - CRITICAL: Use PowerShell call operator (&) when using quoted full paths, OR change directory first and use relative paths
 * - NEVER use relative paths like .\Release\executable.exe without changing directory first
 * - When running executables in PowerShell, always use the complete full path from C:\ OR change to the directory first
 * - Debug versions are in build\Debug\ directory, Release versions in build\Release\ directory
 *
 * Problem Resolution Policy:
 * - Never mark problems as "solved" or "fixed" until user explicitly confirms the fix works
 * - Always test end-to-end functionality before claiming success
 * - If tests fail or user reports continued issues, acknowledge the problem persists
 * - Only declare success when: (1) All tests pass in Docker CI, OR (2) User explicitly confirms the fix works
 * - Use emojis appropriately to indicate status: ✅ for done, 🚧 for in-progress, ⚠️ for warnings, but not in code
 * 
 * FINAL CI/CD POLICY - MASTER MERGE REQUIREMENTS:
 * - MANDATORY: ALL Docker CI tests must pass before any merge to master/main
 * - ZERO TOLERANCE: No exceptions for failed CI tests - automatic PR rejection
 * - DOCKER VALIDATION: Use "docker build -f Dockerfile.ci --target ci-test" for validation
 * - CREDENTIAL TESTING: All integration tests must run with real Coinbase credentials
 * - ARCHITECTURE SUPPORT: Both Windows development and Linux production must work
 * - TEST COVERAGE: Minimum 90% test success rate (currently 15/16 = 94%)
 * - DOCUMENTATION: README.md, TODO.md, and MERGE-CHECKLIST.md must be current
 * - PRODUCTION READY: Every master merge must be deployable to Unraid Docker immediately
 * 
 * GitHub Actions & Continuous Integration:
 * - ALL DTC protocol functions must have automated test coverage
 * - Tests must run against real Coinbase data (not mock data) where possible
 * - Failed tests should block merges to main branch
 * - Test matrix should cover all supported DTC message types
 * - Integration tests must validate end-to-end data flow from Coinbase API through DTC protocol to client
 * - Performance benchmarks for latency-critical operations
 * 
 * CI/CD Policy & Pull Request Requirements:
 * - CRITICAL: ALL tests must pass in Docker CI before any PR can be merged to main/master
 * - Development happens on Windows, but CI validation is Linux-based (Ubuntu)
 * - Coinbase API credentials are injected via GitHub Secrets (CDP_API_KEY_ID, CDP_PRIVATE_KEY)
 * - Integration tests run with real Coinbase credentials in CI environment
 * - NO EXCEPTIONS: Failed CI tests = automatic PR rejection
 * - Docker containers must support both architectures (x64 Windows for dev, x64 Linux for production)
 * - Dockerfile.ci is the authoritative CI/CD test configuration
 * - Production deployment target: Docker on Unraid (Linux)
 * 
 * Multi-Architecture Support:
 * - CMakeLists.linux.txt: Linux CI/CD and production builds (no GUI)
 * - CMakeLists.windows.txt: Windows development builds (with GUI)
 * - Main CMakeLists.txt: Platform-agnostic core functionality
 * - ALL code except GUI test client must be cross-platform compatible
 * - Use conditional compilation (#ifdef _WIN32) for platform-specific code
 * - Docker production stage creates minimal runtime image for Unraid deployment
 */
/*
 * Addendum: Inspirations & Strict Separation Enforcement
 *
 * Inspirations/References (review first before implementing):
 *   - https://github.com/anhydrous99/CTrader
 *   - https://github.com/jusbar23/cbpro-cpp
 *   - https://github.com/amirphl/orderbook-merger
 *
 * Enforcement Reminder:
 * - Any new GUI feature MUST consume data exclusively via DTC messages produced by the server.
 * - No GUI-side Coinbase/WebSocket/REST calls are allowed under any circumstances.
 * - All Coinbase interactions occur in the DTC server, then flow to the GUI over the DTC protocol.
 */