import requests
from bs4 import BeautifulSoup

url = 'https://tienganhglobalsuccess.net/ngu-phap-va-bai-tap-thuc-hanh-tieng-anh-lop-6-global-success-unit-6-our-tet-holiday-co-dap-an-1175/?fbclid=IwY2xjawLrIsxleHRuA2FlbQIxMABicmlkETFlVHBScEhieUZOWFo1WVQwAR7ky4rg0BmNla05QIrllmfWdcU2J8oXaN_wt1PkkEhY-H6Sj1r_9Q00VDOI0w_aem_kfj-XPr8ckc549ew8kEL2w'

# Bước 1: Truy cập trang
headers = {
    'User-Agent': 'Mozilla/5.0'
}
resp = requests.get(url, headers=headers)
soup = BeautifulSoup(resp.text, 'html.parser')

# Bước 2: Tìm iframe hoặc embed chứa file PDF
iframe = soup.find('iframe')
pdf_url = iframe['src'] if iframe else None

# Bước 3: Tải file PDF
if pdf_url:
    if pdf_url.startswith('/'):
        from urllib.parse import urljoin
        pdf_url = urljoin(url, pdf_url)

    print(f"Tải từ: {pdf_url}")
    pdf_data = requests.get(pdf_url, headers=headers).content

    with open("output.pdf", "wb") as f:
        f.write(pdf_data)
    print("Đã lưu file PDF thành output.pdf")
else:
    print("Không tìm thấy PDF URL.")
