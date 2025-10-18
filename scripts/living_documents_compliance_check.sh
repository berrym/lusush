#!/bin/bash
# Living Documents Compliance Check
# Verifies all living documents are synchronized with current project status
# Version: 1.0.0
# Date: 2025-10-18

set -e

echo "======================================================================"
echo "Living Documents Compliance Check"
echo "Version: 1.0.0"
echo "Date: $(date '+%Y-%m-%d %H:%M:%S')"
echo "======================================================================"
echo ""

# Configuration
HANDOFF_DOC="AI_ASSISTANT_HANDOFF_DOCUMENT.md"
SUCCESS_TRACKER="docs/lle_specification/LLE_SUCCESS_ENHANCEMENT_TRACKER.md"
IMPLEMENTATION_GUIDE="docs/lle_specification/LLE_IMPLEMENTATION_GUIDE.md"
DEVELOPMENT_STRATEGY="docs/lle_specification/LLE_DEVELOPMENT_STRATEGY.md"

VIOLATIONS=0

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if we're in the right directory
if [ ! -f "$HANDOFF_DOC" ]; then
    echo -e "${RED}✗ ERROR: Not in project root directory (AI_ASSISTANT_HANDOFF_DOCUMENT.md not found)${NC}"
    exit 1
fi

echo "Checking core living documents..."
echo ""

# Function to extract status from documents
extract_status() {
    local file=$1
    grep -E "^\*\*Status\*\*:|^**Status**:" "$file" | head -1 || echo "Status: NOT FOUND"
}

# Function to extract date from documents
extract_date() {
    local file=$1
    grep -E "^\*\*Date\*\*:|^**Date**:|^\*\*Last Update\*\*:|^**Last Updated**:" "$file" | head -1 || echo "Date: NOT FOUND"
}

# Check 1: All core living documents exist
echo "--- Check 1: Core Living Documents Existence ---"
MISSING=0

for doc in "$HANDOFF_DOC" "$SUCCESS_TRACKER" "$IMPLEMENTATION_GUIDE" "$DEVELOPMENT_STRATEGY"; do
    if [ -f "$doc" ]; then
        echo -e "${GREEN}✓${NC} Found: $doc"
    else
        echo -e "${RED}✗${NC} Missing: $doc"
        MISSING=$((MISSING + 1))
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
done

echo ""
if [ $MISSING -eq 0 ]; then
    echo -e "${GREEN}✓ All core living documents exist${NC}"
else
    echo -e "${RED}✗ VIOLATION: $MISSING core living documents missing${NC}"
fi
echo ""

# If documents are missing, we can't do further checks
if [ $MISSING -gt 0 ]; then
    echo "Cannot perform further checks due to missing documents."
    exit 1
fi

# Check 2: Extract and display status from each document
echo "--- Check 2: Document Status Consistency ---"
echo ""

HANDOFF_STATUS=$(extract_status "$HANDOFF_DOC")
TRACKER_STATUS=$(extract_status "$SUCCESS_TRACKER")
GUIDE_STATUS=$(extract_status "$IMPLEMENTATION_GUIDE")
STRATEGY_STATUS=$(extract_status "$DEVELOPMENT_STRATEGY")

echo "Status in each document:"
echo "  Handoff:    $HANDOFF_STATUS"
echo "  Tracker:    $TRACKER_STATUS"
echo "  Guide:      $GUIDE_STATUS"
echo "  Strategy:   $STRATEGY_STATUS"
echo ""

# Check if all documents contain RECOVERY MODE or none do
HANDOFF_RECOVERY=$(grep -c "RECOVERY MODE" "$HANDOFF_DOC" || echo "0")
TRACKER_RECOVERY=$(grep -c "RECOVERY MODE" "$SUCCESS_TRACKER" || echo "0")
GUIDE_RECOVERY=$(grep -c "RECOVERY MODE" "$IMPLEMENTATION_GUIDE" || echo "0")
STRATEGY_RECOVERY=$(grep -c "RECOVERY MODE" "$DEVELOPMENT_STRATEGY" || echo "0")

echo "Recovery mode mentions:"
echo "  Handoff:    $HANDOFF_RECOVERY"
echo "  Tracker:    $TRACKER_RECOVERY"
echo "  Guide:      $GUIDE_RECOVERY"
echo "  Strategy:   $STRATEGY_RECOVERY"
echo ""

# All should have recovery mode or none should
if [ "$HANDOFF_RECOVERY" -gt 0 ]; then
    # Handoff has recovery mode, others should too
    if [ "$TRACKER_RECOVERY" -eq 0 ] || [ "$GUIDE_RECOVERY" -eq 0 ] || [ "$STRATEGY_RECOVERY" -eq 0 ]; then
        echo -e "${RED}✗ VIOLATION: Recovery mode status inconsistent across documents${NC}"
        echo "  Handoff document indicates recovery mode, but other documents don't"
        VIOLATIONS=$((VIOLATIONS + 1))
    else
        echo -e "${GREEN}✓ Recovery mode status consistent across all documents${NC}"
    fi
else
    # Handoff doesn't have recovery mode, others shouldn't either
    if [ "$TRACKER_RECOVERY" -gt 0 ] || [ "$GUIDE_RECOVERY" -gt 0 ] || [ "$STRATEGY_RECOVERY" -gt 0 ]; then
        echo -e "${RED}✗ VIOLATION: Recovery mode status inconsistent across documents${NC}"
        echo "  Handoff document doesn't indicate recovery mode, but other documents do"
        VIOLATIONS=$((VIOLATIONS + 1))
    else
        echo -e "${GREEN}✓ No recovery mode (status consistent across all documents)${NC}"
    fi
fi
echo ""

# Check 3: Extract and compare dates
echo "--- Check 3: Document Date Synchronization ---"
echo ""

HANDOFF_DATE=$(extract_date "$HANDOFF_DOC")
TRACKER_DATE=$(extract_date "$SUCCESS_TRACKER")
GUIDE_DATE=$(extract_date "$IMPLEMENTATION_GUIDE")
STRATEGY_DATE=$(extract_date "$DEVELOPMENT_STRATEGY")

echo "Dates in each document:"
echo "  Handoff:    $HANDOFF_DATE"
echo "  Tracker:    $TRACKER_DATE"
echo "  Guide:      $GUIDE_DATE"
echo "  Strategy:   $STRATEGY_DATE"
echo ""

# Extract just the date portion (YYYY-MM-DD)
HANDOFF_DATE_ONLY=$(echo "$HANDOFF_DATE" | grep -oE '20[0-9]{2}-[0-9]{2}-[0-9]{2}' | head -1 || echo "NONE")
TRACKER_DATE_ONLY=$(echo "$TRACKER_DATE" | grep -oE '20[0-9]{2}-[0-9]{2}-[0-9]{2}' | head -1 || echo "NONE")
GUIDE_DATE_ONLY=$(echo "$GUIDE_DATE" | grep -oE '20[0-9]{2}-[0-9]{2}-[0-9]{2}' | head -1 || echo "NONE")
STRATEGY_DATE_ONLY=$(echo "$STRATEGY_DATE" | grep -oE '20[0-9]{2}-[0-9]{2}-[0-9]{2}' | head -1 || echo "NONE")

# Check if dates are reasonably synchronized (within 7 days)
# For simplicity, we'll just check if they're all the same or within a reasonable range
if [ "$HANDOFF_DATE_ONLY" = "NONE" ]; then
    echo -e "${YELLOW}⚠ WARNING: Could not extract date from handoff document${NC}"
else
    # Check if other documents are updated to at least the handoff date
    # For now, just warn if dates differ significantly
    if [ "$TRACKER_DATE_ONLY" != "$HANDOFF_DATE_ONLY" ] || \
       [ "$GUIDE_DATE_ONLY" != "$HANDOFF_DATE_ONLY" ] || \
       [ "$STRATEGY_DATE_ONLY" != "$HANDOFF_DATE_ONLY" ]; then
        echo -e "${YELLOW}⚠ WARNING: Document dates are not synchronized${NC}"
        echo "  Consider updating all documents to match current status"
        echo "  This is not a violation if documents were intentionally updated at different times"
    else
        echo -e "${GREEN}✓ All documents have the same date ($HANDOFF_DATE_ONLY)${NC}"
    fi
fi
echo ""

# Check 4: Check for BLOCKER issues consistency
echo "--- Check 4: BLOCKER Issues Consistency ---"
echo ""

HANDOFF_BLOCKER=$(grep -c "BLOCKER" "$HANDOFF_DOC" || echo "0")
TRACKER_BLOCKER=$(grep -c "BLOCKER" "$SUCCESS_TRACKER" || echo "0")
GUIDE_BLOCKER=$(grep -c "BLOCKER" "$IMPLEMENTATION_GUIDE" || echo "0")
STRATEGY_BLOCKER=$(grep -c "BLOCKER" "$DEVELOPMENT_STRATEGY" || echo "0")

echo "BLOCKER issue mentions:"
echo "  Handoff:    $HANDOFF_BLOCKER"
echo "  Tracker:    $TRACKER_BLOCKER"
echo "  Guide:      $GUIDE_BLOCKER"
echo "  Strategy:   $STRATEGY_BLOCKER"
echo ""

if [ "$HANDOFF_BLOCKER" -gt 0 ]; then
    # Handoff mentions blockers, others should too
    if [ "$TRACKER_BLOCKER" -eq 0 ] || [ "$GUIDE_BLOCKER" -eq 0 ]; then
        echo -e "${YELLOW}⚠ WARNING: BLOCKER issues mentioned in handoff but not in tracker/guide${NC}"
        echo "  Consider updating tracker and guide documents"
    else
        echo -e "${GREEN}✓ BLOCKER issues consistently documented${NC}"
    fi
else
    echo -e "${GREEN}✓ No BLOCKER issues mentioned (consistent)${NC}"
fi
echo ""

# Check 5: Check for success probability consistency (if not in recovery)
echo "--- Check 5: Success Probability Consistency ---"
echo ""

if [ "$HANDOFF_RECOVERY" -eq 0 ]; then
    # Not in recovery mode, check for success probability
    HANDOFF_SUCCESS=$(grep -oE '[0-9]{1,3}%' "$HANDOFF_DOC" | head -1 || echo "NONE")
    TRACKER_SUCCESS=$(grep -oE '[0-9]{1,3}%' "$SUCCESS_TRACKER" | head -1 || echo "NONE")

    echo "Success probability mentions:"
    echo "  Handoff:    $HANDOFF_SUCCESS"
    echo "  Tracker:    $TRACKER_SUCCESS"
    echo ""

    if [ "$HANDOFF_SUCCESS" != "NONE" ] && [ "$TRACKER_SUCCESS" != "NONE" ]; then
        if [ "$HANDOFF_SUCCESS" != "$TRACKER_SUCCESS" ]; then
            echo -e "${YELLOW}⚠ WARNING: Success probability differs between handoff and tracker${NC}"
            echo "  Handoff: $HANDOFF_SUCCESS, Tracker: $TRACKER_SUCCESS"
        else
            echo -e "${GREEN}✓ Success probability consistent: $HANDOFF_SUCCESS${NC}"
        fi
    else
        echo -e "${YELLOW}⚠ Could not determine success probability${NC}"
    fi
else
    echo -e "${YELLOW}⚠ In recovery mode - success probability not applicable${NC}"
fi
echo ""

# Summary
echo "======================================================================"
echo "COMPLIANCE CHECK SUMMARY"
echo "======================================================================"
echo ""

if [ $VIOLATIONS -eq 0 ]; then
    echo -e "${GREEN}✓ PASS: All compliance checks passed${NC}"
    echo ""
    echo "Living documents are synchronized and consistent."
    echo ""
    exit 0
else
    echo -e "${RED}✗ FAIL: $VIOLATIONS violation(s) detected${NC}"
    echo ""
    echo "Living documents have inconsistencies that must be resolved."
    echo ""
    echo "Actions required:"
    echo "1. Review the violations listed above"
    echo "2. Update all living documents to reflect current project status"
    echo "3. Ensure MANDATORY cross-document updates are performed"
    echo "4. Re-run this check to verify compliance"
    echo ""
    exit 1
fi
