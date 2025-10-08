#!/bin/bash
# Shell script to clean, build and run PFSP-SDST project
# Usage: ./build_and_run.sh [data_file] [algorithm]

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Set default parameters if not provided
DATA_FILE=${1:-"../data/10_2_2_2.txt"}
ALGORITHM=${2:-"test_algorithm"}

echo -e "${GREEN}=== PFSP-SDST Build and Run Script ===${NC}"
echo -e "${YELLOW}Data file: $DATA_FILE${NC}"
echo -e "${YELLOW}Algorithm: $ALGORITHM${NC}"
echo

echo -e "${YELLOW}1. Cleaning previous build...${NC}"
if make clean; then
    echo -e "${GREEN}✓ Clean completed successfully${NC}"
else
    echo -e "${RED}✗ Clean failed!${NC}"
    exit 1
fi
echo

echo -e "${YELLOW}2. Building project...${NC}"
if make build; then
    echo -e "${GREEN}✓ Build completed successfully${NC}"
else
    echo -e "${RED}✗ Build failed!${NC}"
    exit 1
fi
echo

echo -e "${YELLOW}3. Running application...${NC}"
if [ ! -f "$DATA_FILE" ]; then
    echo -e "${RED}✗ Data file '$DATA_FILE' not found!${NC}"
    exit 1
fi

if ./bin/pfsp_sdst "$DATA_FILE" "$ALGORITHM"; then
    echo
    echo -e "${GREEN}✓ Application completed successfully${NC}"
else
    echo
    echo -e "${RED}✗ Application execution failed!${NC}"
    exit 1
fi

echo
echo -e "${GREEN}=== All operations completed successfully! ===${NC}"
