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
 * Use emojis appropriately to indicate status: ✅ for done, 🚧 for in-progress, ⚠️ for warnings, but not in code
 * 
 * GitHub Actions & Continuous Integration:
 * - ALL DTC protocol functions must have automated test coverage
 * - Tests must run against real Coinbase data (not mock data) where possible
 * - Failed tests should block merges to main branch
 * - Test matrix should cover all supported DTC message types
 * - Integration tests must validate end-to-end data flow from Coinbase API through DTC protocol to client
 * - Performance benchmarks for latency-critical operations
 */