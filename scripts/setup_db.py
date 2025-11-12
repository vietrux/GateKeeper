#!/usr/bin/env python3
"""
Database setup script for GateKeeper system
Creates the database schema and optionally adds sample data
"""

import sys
import os
import sqlite3
from pathlib import Path

# Add parent directory to path to import config
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from config.settings import DATABASE_PATH, DATABASE_DIR


def create_database():
    """Create the database schema"""
    print(f"Creating database at: {DATABASE_PATH}")

    # Ensure directory exists
    os.makedirs(DATABASE_DIR, exist_ok=True)

    try:
        conn = sqlite3.connect(DATABASE_PATH)
        cursor = conn.cursor()

        # Create plates table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS plates (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            plate_number TEXT UNIQUE NOT NULL,
            added_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """)
        print("✓ Created 'plates' table")

        # Create movement_log table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS movement_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            plate_number TEXT NOT NULL,
            action TEXT NOT NULL,
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """)
        print("✓ Created 'movement_log' table")

        conn.commit()
        conn.close()

        print("\n✓ Database created successfully!")
        return True

    except Exception as e:
        print(f"\n✗ Error creating database: {e}")
        return False


def add_sample_data():
    """Add sample license plates for testing"""
    sample_plates = ["29A12345", "30B67890", "51C11111"]

    try:
        conn = sqlite3.connect(DATABASE_PATH)
        cursor = conn.cursor()

        for plate in sample_plates:
            try:
                cursor.execute("INSERT INTO plates (plate_number) VALUES (?)", (plate,))
                print(f"✓ Added sample plate: {plate}")
            except sqlite3.IntegrityError:
                print(f"⚠ Plate {plate} already exists, skipping")

        conn.commit()
        conn.close()

        print("\n✓ Sample data added successfully!")
        return True

    except Exception as e:
        print(f"\n✗ Error adding sample data: {e}")
        return False


if __name__ == "__main__":
    print("=" * 50)
    print("GateKeeper Database Setup")
    print("=" * 50)
    print()

    # Create database
    if create_database():
        # Ask if user wants to add sample data
        response = input("\nAdd sample license plates for testing? (y/n): ").lower()
        if response == "y":
            add_sample_data()

    print("\nSetup complete!")
