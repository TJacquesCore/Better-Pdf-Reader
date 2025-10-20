import sys
import os
import io
import multiprocessing
import img2pdf
import fitz
from PIL import Image, ImageOps

def extraire_bookmarks_hierarchiques(pdf_path):
    try:
        pdf = fitz.open(pdf_path)
        toc = pdf.get_toc()
        pdf.close()
        
        if toc:
            print(f" {len(toc)} bookmarks hierarchiques trouves")
            for level, title, page, *rest in toc:
                indent = "  " * (level - 1)
                print(f"   {indent} {title} (page {page})")
            return toc
        else:
            print("  Aucun bookmark hierarchique trouve")
            return None
            
    except Exception as e:
        print(f" Impossible d extraire les bookmarks: {e}")
        return None

def creer_pdf_avec_bookmarks_fitz(images_data, output_pdf, bookmarks_hierarchiques):
    try:
        pdf_bytes = img2pdf.convert(images_data)
        
        doc = fitz.open("pdf", pdf_bytes)
        
        if bookmarks_hierarchiques:
            print(" Reconstruction de la hierarchie des bookmarks...")
            
            bookmarks_ajustes = []
            for item in bookmarks_hierarchiques:
                level, title, page_num = item[0], item[1], item[2]
                page_target = min(page_num - 1, len(images_data) - 1)
                if page_target >= 0:
                    new_item = list(item)
                    new_item[2] = page_target + 1
                    bookmarks_ajustes.append(tuple(new_item))
            
            doc.set_toc(bookmarks_ajustes)
            print(" Hierarchie des bookmarks restauree")
        
        doc.save(output_pdf)
        doc.close()
        return True
        
    except Exception as e:
        print(f" Erreur lors de la creation du PDF avec bookmarks: {e}")
        return False

def traiter_page(args):
    pdf_bytes, page_num, dpi, quality = args
    try:
        pdf = fitz.open("pdf", pdf_bytes)
        page = pdf.load_page(page_num)
        pix = page.get_pixmap(matrix=fitz.Matrix(dpi/72, dpi/72), alpha=False)
        
        img_bytes = pix.tobytes("jpeg", jpg_quality=quality)
        
        image = Image.open(io.BytesIO(img_bytes))
        image_inverse = ImageOps.invert(image)
        
        img_buffer = io.BytesIO()
        image_inverse.save(img_buffer, format="JPEG", quality=quality, optimize=True)
        
        pdf.close()
        return (page_num, img_buffer.getvalue())
        
    except Exception as e:
        print(f"Erreur page {page_num}: {e}")
        return (page_num, None)

def main():
    if len(sys.argv) != 2:
        print("Usage: python script.py <fichier.pdf>")
        sys.exit(1)
    
    pdf_path = sys.argv[1]
    
    if not os.path.exists(pdf_path):
        print(f" Erreur: Le fichier '{pdf_path}' n existe pas.")
        sys.exit(1)
    
    if not pdf_path.lower().endswith('.pdf'):
        print(" Erreur: Le fichier doit etre un PDF.")
        sys.exit(1)
    
    try:
        print(" Extraction des bookmarks hierarchiques...")
        bookmarks = extraire_bookmarks_hierarchiques(pdf_path)
        
        with open(pdf_path, "rb") as f:
            pdf_bytes = f.read()
        
        pdf = fitz.open("pdf", pdf_bytes)
        total_pages = len(pdf)
        pdf.close()
        
        print(f" Traitement de {total_pages} pages...")
        
        dpi = 150
        quality = 80
        
        tasks = [(pdf_bytes, i, dpi, quality) for i in range(total_pages)]
        
        nb_coeurs = min(multiprocessing.cpu_count(), 4)
        if total_pages < 10:
            nb_coeurs = 1
            
        print(f" Utilisation de {nb_coeurs} processus")
        
        images_ordonnees = []
        
        if nb_coeurs > 1:
            with multiprocessing.Pool(processes=nb_coeurs) as pool:
                results = pool.map(traiter_page, tasks)
            
            for page_num, img_data in sorted(results, key=lambda x: x[0]):
                if img_data is not None:
                    images_ordonnees.append(img_data)
                    print(f" Page {page_num + 1} traitee")
        else:
            for page_num in range(total_pages):
                img_data = traiter_page((pdf_bytes, page_num, dpi, quality))
                if img_data[1] is not None:
                    images_ordonnees.append(img_data[1])
                print(f" Page {page_num + 1} traitee")
        
        # MODIFICATION IMPORTANTE ICI :
        # Utiliser le répertoire de travail courant au lieu du répertoire du PDF
        current_dir = os.getcwd()
        dark_dir = os.path.join(current_dir, "dark")
        
        if not os.path.exists(dark_dir):
            print(f" Creation du dossier: {dark_dir}")
            os.makedirs(dark_dir)
        
        pdf_filename = os.path.basename(pdf_path)
        output_pdf = os.path.join(dark_dir, pdf_filename)
        
        print(f" Sauvegarde dans: {output_pdf}")
        
        if bookmarks:
            success = creer_pdf_avec_bookmarks_fitz(images_ordonnees, output_pdf, bookmarks)
            if not success:
                print(" Retour a la creation basique sans bookmarks...")
                with open(output_pdf, "wb") as f:
                    f.write(img2pdf.convert(images_ordonnees))
        else:
            with open(output_pdf, "wb") as f:
                f.write(img2pdf.convert(images_ordonnees))
        
        print(f" PDF dark cree avec succes: {output_pdf}")
        print(f" Statistiques: {total_pages} pages traitees")
        if bookmarks:
            print(f" {len(bookmarks)} bookmarks hierarchiques restaures")
        
    except Exception as e:
        print(f" Erreur critique: {e}")
        sys.exit(1)

if __name__ == "__main__":
    multiprocessing.freeze_support()
    main()