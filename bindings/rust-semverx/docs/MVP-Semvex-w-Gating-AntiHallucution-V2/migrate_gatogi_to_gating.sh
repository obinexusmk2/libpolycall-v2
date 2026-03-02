#!/bin/bash
# migrate_gatogi_to_gating.sh
# OBINexus Gating Terminology Correction Script
# Corrects all instances of "gatogi" → "gating" across the project

set -e  # Exit on error

echo "╔════════════════════════════════════════════════════════════╗"
echo "║   OBINexus Gating Terminology Correction Script          ║"
echo "║   gatogi → gating (Computational Cognition Framework)    ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Backup directory
BACKUP_DIR="./backup_$(date +%Y%m%d_%H%M%S)"

echo -e "${YELLOW}Step 1: Creating backup...${NC}"
mkdir -p "$BACKUP_DIR"

# Find all files that might contain "gatogi"
FILES_TO_CHECK=$(find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.rs" -o -name "*.md" -o -name "*.txt" \) ! -path "*/\.*" ! -path "*/backup_*/*")

# Count matches before correction
TOTAL_MATCHES=$(grep -o -i "gatogi" $FILES_TO_CHECK 2>/dev/null | wc -l)

echo -e "${GREEN}Found ${TOTAL_MATCHES} instances of 'gatogi' to correct${NC}"
echo ""

# Backup affected files
echo -e "${YELLOW}Step 2: Backing up affected files...${NC}"
for file in $FILES_TO_CHECK; do
    if grep -q -i "gatogi" "$file" 2>/dev/null; then
        cp "$file" "$BACKUP_DIR/"
        echo "  Backed up: $file"
    fi
done
echo ""

# Function to perform replacements
perform_replacements() {
    local file=$1
    
    # Case-sensitive replacements (order matters!)
    sed -i 's/GATOGI_/GATING_/g' "$file"              # GATOGI_VALID → GATING_VALID
    sed -i 's/GatogiContext/GatingContext/g' "$file"   # Type names
    sed -i 's/GatogiResult/GatingResult/g' "$file"     # Type names
    sed -i 's/gatogi_/gating_/g' "$file"              # Function names
    sed -i 's/Gatogi /Gating /g' "$file"              # Comments "Gatogi analysis"
    sed -i 's/\"Gatogi/\"Gating/g' "$file"            # String literals
    sed -i 's/=== Gatogi/=== Gating/g' "$file"        # Printf headers
}

# Step 3: Rename files
echo -e "${YELLOW}Step 3: Renaming files...${NC}"

if [ -f "gatogi_algorithm.c" ]; then
    git mv gatogi_algorithm.c gating_algorithm.c 2>/dev/null || mv gatogi_algorithm.c gating_algorithm.c
    echo -e "${GREEN}  ✓ Renamed: gatogi_algorithm.c → gating_algorithm.c${NC}"
fi

if [ -f "gatogi_mulit_polygon.c" ]; then
    git mv gatogi_mulit_polygon.c gating_multi_pattern.c 2>/dev/null || mv gatogi_mulit_polygon.c gating_multi_pattern.c
    echo -e "${GREEN}  ✓ Renamed: gatogi_mulit_polygon.c → gating_multi_pattern.c${NC}"
fi

echo ""

# Step 4: Correct content in all files
echo -e "${YELLOW}Step 4: Correcting content in all files...${NC}"

for file in $FILES_TO_CHECK; do
    if [ -f "$file" ] && grep -q -i "gatogi" "$file" 2>/dev/null; then
        echo "  Processing: $file"
        perform_replacements "$file"
        echo -e "    ${GREEN}✓ Corrected${NC}"
    fi
done

# Also check the renamed files
if [ -f "gating_algorithm.c" ]; then
    perform_replacements "gating_algorithm.c"
fi
if [ -f "gating_multi_pattern.c" ]; then
    perform_replacements "gating_multi_pattern.c"
fi

echo ""

# Step 5: Update CMakeLists.txt if exists
echo -e "${YELLOW}Step 5: Updating build files...${NC}"

if [ -f "CMakeLists.txt" ]; then
    echo "  Updating CMakeLists.txt..."
    sed -i 's/gatogi_algorithm/gating_algorithm/g' CMakeLists.txt
    sed -i 's/gatogi_multi_polygon/gating_multi_pattern/g' CMakeLists.txt
    echo -e "    ${GREEN}✓ CMakeLists.txt updated${NC}"
fi

if [ -f "Makefile" ]; then
    echo "  Updating Makefile..."
    sed -i 's/gatogi_algorithm/gating_algorithm/g' Makefile
    sed -i 's/gatogi_multi_polygon/gating_multi_pattern/g' Makefile
    echo -e "    ${GREEN}✓ Makefile updated${NC}"
fi

echo ""

# Step 6: Verification
echo -e "${YELLOW}Step 6: Verification...${NC}"

REMAINING_MATCHES=$(find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.rs" -o -name "*.md" \) ! -path "*/\.*" ! -path "*/backup_*/*" -exec grep -l -i "gatogi" {} \; 2>/dev/null | wc -l)

if [ "$REMAINING_MATCHES" -eq 0 ]; then
    echo -e "${GREEN}  ✓ Verification passed: All instances corrected${NC}"
else
    echo -e "${RED}  ⚠ Warning: Found $REMAINING_MATCHES files still containing 'gatogi'${NC}"
    echo "    Files:"
    find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.rs" -o -name "*.md" \) ! -path "*/\.*" ! -path "*/backup_*/*" -exec grep -l -i "gatogi" {} \; 2>/dev/null
fi

echo ""

# Step 7: Summary
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                   Correction Summary                      ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo -e "${GREEN}Corrections completed successfully!${NC}"
echo ""
echo "Summary:"
echo "  • Total instances found: $TOTAL_MATCHES"
echo "  • Remaining instances: $REMAINING_MATCHES"
echo "  • Backup location: $BACKUP_DIR"
echo ""
echo "Changes made:"
echo "  ✓ GatogiContext → GatingContext"
echo "  ✓ GatogiResult → GatingResult"
echo "  ✓ gatogi_* functions → gating_* functions"
echo "  ✓ GATOGI_* enums → GATING_* enums"
echo "  ✓ File renames completed"
echo "  ✓ Build files updated"
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo "  1. Review changes: git diff"
echo "  2. Test build: cmake -B build && cmake --build build"
echo "  3. Run tests: cd build && ctest"
echo "  4. Commit changes:"
echo "     git add -A"
echo "     git commit -m \"fix: correct terminology from 'gatogi' to 'gating'\""
echo ""
echo -e "${GREEN}To restore from backup (if needed):${NC}"
echo "  cp -r $BACKUP_DIR/* ."
echo ""
