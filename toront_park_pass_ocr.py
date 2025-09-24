import re
import pdfplumber

def extract_permit_info(pdf_file):
    text = ""
    with pdfplumber.open(pdf_file) as pdf:
        for page in pdf.pages:
            t = page.extract_text()
            if t:
                text += t + "\n"

    data = {}

    # Dates: capture up to "AM" or "PM"
    valid_from_match = re.search(r"Valid from:\s*([A-Za-z0-9,: ]+(?:AM|PM))", text)
    valid_to_match = re.search(r"Valid to:\s*([A-Za-z0-9,: ]+(?:AM|PM))", text)
    issued_match = re.search(r"Issued:\s*([A-Za-z0-9,: ]+(?:AM|PM))", text)

    data["valid_from"] = valid_from_match.group(1) if valid_from_match else None
    data["valid_to"] = valid_to_match.group(1) if valid_to_match else None
    data["issued"] = issued_match.group(1) if issued_match else None

    # Permit / plate
    permit_match = re.search(r"Permit no\.:\s*([A-Z0-9]+)", text)
    plate_match = re.search(r"Plate no\.:\s*([A-Z0-9]+)", text)
    data["permit_no"] = permit_match.group(1) if permit_match else None
    data["plate_no"] = plate_match.group(1) if plate_match else None

    # Barcode number (standalone digits)
    barcode_match = re.search(r"(?m)^\s*([0-9]{3,10})\s*$", text)
    data["barcode_number"] = barcode_match.group(1) if barcode_match else None

    return data


if __name__ == "__main__":
    pdf_name = "permit.pdf"  # PDF in same folder
    info = extract_permit_info(pdf_name)
    for k, v in info.items():
        print(f"{k}: {v}")
