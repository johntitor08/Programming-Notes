import React, { useState, useEffect } from 'react';
import { Book, Scan, Plus, Search, Trash2, Edit2, X, Check } from 'lucide-react';

export default function LibraryApp() {
  const [books, setBooks] = useState([]);
  const [showAddForm, setShowAddForm] = useState(false);
  const [searchTerm, setSearchTerm] = useState('');
  const [editingId, setEditingId] = useState(null);
  const [formData, setFormData] = useState({
    barcode: '',
    title: '',
    author: '',
    isbn: '',
    year: '',
    category: ''
  });

  useEffect(() => {
    loadBooks();
  }, []);

  const loadBooks = async () => {
    try {
      const keys = await window.storage.list('book:');
      if (keys && keys.keys) {
        const loadedBooks = await Promise.all(
          keys.keys.map(async (key) => {
            try {
              const result = await window.storage.get(key);
              return result ? JSON.parse(result.value) : null;
            } catch {
              return null;
            }
          })
        );
        setBooks(loadedBooks.filter(Boolean));
      }
    } catch (error) {
      console.error('Kitaplar yüklenemedi:', error);
    }
  };

  const generateBarcode = () => {
    return 'BK' + Date.now() + Math.floor(Math.random() * 1000);
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    
    const bookData = {
      ...formData,
      barcode: formData.barcode || generateBarcode(),
      id: editingId || Date.now().toString(),
      addedDate: editingId ? books.find(b => b.id === editingId)?.addedDate : new Date().toISOString()
    };

    try {
      await window.storage.set(`book:${bookData.id}`, JSON.stringify(bookData));
      await loadBooks();
      resetForm();
    } catch (error) {
      console.error('Kitap kaydedilemedi:', error);
      alert('Kitap kaydedilirken bir hata oluştu');
    }
  };

  const handleDelete = async (id) => {
    if (window.confirm('Bu kitabı silmek istediğinizden emin misiniz?')) {
      try {
        await window.storage.delete(`book:${id}`);
        await loadBooks();
      } catch (error) {
        console.error('Kitap silinemedi:', error);
      }
    }
  };

  const handleEdit = (book) => {
    setFormData({
      barcode: book.barcode,
      title: book.title,
      author: book.author,
      isbn: book.isbn,
      year: book.year,
      category: book.category
    });
    setEditingId(book.id);
    setShowAddForm(true);
  };

  const resetForm = () => {
    setFormData({
      barcode: '',
      title: '',
      author: '',
      isbn: '',
      year: '',
      category: ''
    });
    setEditingId(null);
    setShowAddForm(false);
  };

  const filteredBooks = books.filter(book =>
    book.title.toLowerCase().includes(searchTerm.toLowerCase()) ||
    book.author.toLowerCase().includes(searchTerm.toLowerCase()) ||
    book.barcode.toLowerCase().includes(searchTerm.toLowerCase()) ||
    book.isbn.toLowerCase().includes(searchTerm.toLowerCase())
  );

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 to-indigo-100 p-4">
      <div className="max-w-6xl mx-auto">
        {/* Header */}
        <div className="bg-white rounded-lg shadow-lg p-6 mb-6">
          <div className="flex items-center justify-between mb-4">
            <div className="flex items-center gap-3">
              <Book className="w-8 h-8 text-indigo-600" />
              <h1 className="text-3xl font-bold text-gray-800">Kütüphane Yönetim Sistemi</h1>
            </div>
            <button
              onClick={() => setShowAddForm(!showAddForm)}
              className="flex items-center gap-2 bg-indigo-600 text-white px-4 py-2 rounded-lg hover:bg-indigo-700 transition"
            >
              {showAddForm ? <X className="w-5 h-5" /> : <Plus className="w-5 h-5" />}
              {showAddForm ? 'İptal' : 'Yeni Kitap'}
            </button>
          </div>

          {/* Search */}
          <div className="relative">
            <Search className="absolute left-3 top-3 w-5 h-5 text-gray-400" />
            <input
              type="text"
              placeholder="Kitap ara (başlık, yazar, barkod, ISBN)..."
              value={searchTerm}
              onChange={(e) => setSearchTerm(e.target.value)}
              ;
                const [books, setBooks] = useState<Book[]>([]);
                const [showAddForm, setShowAddForm] = useState<boolean>(false);
                const [searchTerm, setSearchTerm] = useState<string>('');
                const [editingId, setEditingId] = useState<string | null>(null);
                const [formData, setFormData] = useState<FormData>({
              className="w-full pl-10 pr-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-transparent"
            />
          </div>
        </div>

        {/* Add/Edit Form */}
        {showAddForm && (
          <div className="bg-white rounded-lg shadow-lg p-6 mb-6">
            <h2 className="text-xl font-bold text-gray-800 mb-4">
              {editingId ? 'Kitabı Düzenle' : 'Yeni Kitap Ekle'}
            </h2>
            <form onSubmit={handleSubmit} className="grid grid-cols-1 md:grid-cols-2 gap-4">
              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  Barkod
                </label>
                <div className="flex gap-2">
                  <input
                    type="text"
                    value={formData.barcode}
                    onChange={(e) => setFormData({...formData, barcode: e.target.value})}
                    placeholder="Otomatik oluşturulacak"
                    className="flex-1 px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-indigo-500"
                  />
                  <button
                    type="button"
                    onClick={() => setFormData({...formData, barcode: generateBarcode()})}
                    className="px-3 py-2 bg-gray-200 rounded-lg hover:bg-gray-300"
                  >
                    <Scan className="w-5 h-5" />
                  </button>
                </div>
              </div>

              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  Kitap Başlığı *
                </label>
                <input
                  type="text"
                  required
                  value={formData.title}
                  onChange={(e) => setFormData({...formData, title: e.target.value})}
                  className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-indigo-500"
                />
              </div>

              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  Yazar *
                </label>
                <input
                  type="text"
                  required
                  value={formData.author}
                  onChange={(e) => setFormData({...formData, author: e.target.value})}
                  className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-indigo-500"
                />
              </div>

              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  ISBN
                </label>
                <input
                  type="text"
                  value={formData.isbn}
                  onChange={(e) => setFormData({...formData, isbn: e.target.value})}
                  className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-indigo-500"
                />
              </div>

              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  Yayın Yılı
                </label>
                <input
                  type="text"
                  value={formData.year}
                  onChange={(e) => setFormData({...formData, year: e.target.value})}
                  className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-indigo-500"
                />
              </div>

              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  Kategori
                </label>
                <input
                  type="text"
                  value={formData.category}
                  onChange={(e) => setFormData({...formData, category: e.target.value})}
                  placeholder="Roman, Bilim, Tarih, vb."
                  className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-indigo-500"
                />
              </div>

              <div className="md:col-span-2 flex gap-2 justify-end">
                <button
                  type="button"
                  onClick={resetForm}
                  className="px-4 py-2 border border-gray-300 rounded-lg hover:bg-gray-50"
                >
                  İptal
                </button>
                <button
                  type="submit"
                  className="flex items-center gap-2 px-4 py-2 bg-indigo-600 text-white rounded-lg hover:bg-indigo-700"
                >
                  <Check className="w-5 h-5" />
                  {editingId ? 'Güncelle' : 'Kaydet'}
                </button>
              </div>
            </form>
          </div>
        )}

        {/* Books List */}
        <div className="bg-white rounded-lg shadow-lg p-6">
          <h2 className="text-xl font-bold text-gray-800 mb-4">
            Kitaplar ({filteredBooks.length})
          </h2>
          
          {filteredBooks.length === 0 ? (
            <div className="text-center py-12 text-gray-500">
              <Book className="w-16 h-16 mx-auto mb-4 text-gray-300" />
              <p>Henüz kitap eklenmemiş</p>
            </div>
          ) : (
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
              {filteredBooks.map((book) => (
                <div
                  key={book.id}
                  className="border border-gray-200 rounded-lg p-4 hover:shadow-md transition"
                >
                  <div className="flex justify-between items-start mb-2">
                    <div className="flex-1">
                      <h3 className="font-bold text-gray-800 mb-1">{book.title}</h3>
                      <p className="text-sm text-gray-600">{book.author}</p>
                    </div>
                    <div className="flex gap-1">
                      <button
                        onClick={() => handleEdit(book)}
                        className="p-1 text-blue-600 hover:bg-blue-50 rounded"
                      >
                        <Edit2 className="w-4 h-4" />
                      </button>
                      <button
                        onClick={() => handleDelete(book.id)}
                        className="p-1 text-red-600 hover:bg-red-50 rounded"
                      >
                        <Trash2 className="w-4 h-4" />
                      </button>
                    </div>
                  </div>
                  
                  <div className="space-y-1 text-sm text-gray-600">
                    <div className="flex items-center gap-2">
                      <Scan className="w-4 h-4" />
                      <span className="font-mono">{book.barcode}</span>
                    </div>
                    {book.isbn && <p>ISBN: {book.isbn}</p>}
                    {book.year && <p>Yıl: {book.year}</p>}
                    {book.category && (
                      <span className="inline-block px-2 py-1 bg-indigo-100 text-indigo-700 rounded text-xs">
                        {book.category}
                      </span>
                    )}
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}