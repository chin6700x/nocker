<?php
/**
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 * 
 * ⚡ NOCKER: Not Docker · Full-Stack Web + PHP + Database Live Demonstration
 */

$start_time = microtime(true);

// 1. Database Setup in Volume Mount (/data)
$db_dir = __DIR__ . '/../data';
if (!is_dir($db_dir)) {
    @mkdir($db_dir, 0777, true);
}
$db_file = $db_dir . '/silicon_store.db';

try {
    $pdo = new PDO("sqlite:" . $db_file);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    // Initialize Schema if not exists
    $pdo->exec("
        CREATE TABLE IF NOT EXISTS products (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            category TEXT NOT NULL,
            price REAL NOT NULL,
            stock INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    ");

    // Seed mock data if table is completely empty
    $count = $pdo->query("SELECT COUNT(*) FROM products")->fetchColumn();
    if ($count == 0) {
        $seeds = [
            ['MacBook Pro M4 Max (128GB)', 'Hardware', 149900.00, 5],
            ['Zephyr Silicon Terminal Engine', 'Software', 0.00, 999],
            ['Nocker Studio Pro', 'DevOps', 0.00, 888],
            ['Apple Studio Display 5K', 'Hardware', 54900.00, 12],
            ['APFS Ultra NVMe CoW Storage', 'Cloud', 12500.00, 30]
        ];
        $stmt = $pdo->prepare("INSERT INTO products (name, category, price, stock) VALUES (?, ?, ?, ?)");
        foreach ($seeds as $item) {
            $stmt->execute($item);
        }
    }

    // Handle Insert New Record
    $action_msg = "";
    if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['action'])) {
        if ($_POST['action'] === 'add') {
            $name = trim($_POST['name'] ?? '');
            $category = trim($_POST['category'] ?? 'General');
            $price = floatval($_POST['price'] ?? 0);
            $stock = intval($_POST['stock'] ?? 0);

            if (!empty($name)) {
                $stmt = $pdo->prepare("INSERT INTO products (name, category, price, stock) VALUES (?, ?, ?, ?)");
                $stmt->execute([$name, $category, $price, $stock]);
                $action_msg = "✓ Added product '$name' into Container Database successfully!";
            }
        } elseif ($_POST['action'] === 'delete') {
            $del_id = intval($_POST['id'] ?? 0);
            if ($del_id > 0) {
                $stmt = $pdo->prepare("DELETE FROM products WHERE id = ?");
                $stmt->execute([$del_id]);
                $action_msg = "✓ Product ID #$del_id deleted from Database.";
            }
        }
    }

    // Fetch all products
    $stmt = $pdo->query("SELECT * FROM products ORDER BY id DESC");
    $products = $stmt->fetchAll(PDO::FETCH_ASSOC);

} catch (Exception $e) {
    die("Database Connection Error: " . htmlspecialchars($e->getMessage()));
}

$query_time = round((microtime(true) - $start_time) * 1000, 3);
?>
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Nocker · Not Docker · Full-Stack Live Demo</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;500;700&family=Plus+Jakarta+Sans:wght@400;500;600;700;800&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg: #070a12;
      --card: rgba(18, 26, 46, 0.75);
      --border: rgba(255, 255, 255, 0.08);
      --text: #f1f5f9;
      --muted: #94a3b8;
      --dim: #64748b;
      --cyan: #00f2fe;
      --blue: #3b82f6;
      --green: #10b981;
      --amber: #f59e0b;
      --red: #ef4444;
      --purple: #a855f7;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: 'Plus Jakarta Sans', sans-serif;
      background: var(--bg);
      background-image: radial-gradient(circle at 50% 0%, #152244 0%, #070a12 75%);
      color: var(--text);
      min-height: 100vh;
      line-height: 1.5;
      padding-bottom: 5rem;
    }
    .header {
      padding: 1.5rem 2.5rem;
      display: flex;
      justify-content: space-between;
      align-items: center;
      background: rgba(7, 10, 18, 0.85);
      backdrop-filter: blur(20px);
      border-bottom: 1px solid var(--border);
      position: sticky;
      top: 0;
      z-index: 100;
    }
    .brand {
      display: flex;
      align-items: center;
      gap: 1rem;
    }
    .brand-icon {
      width: 44px;
      height: 44px;
      background: linear-gradient(135deg, var(--cyan), var(--blue));
      border-radius: 12px;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 1.5rem;
      box-shadow: 0 0 25px rgba(0, 242, 254, 0.3);
    }
    .brand-title {
      font-size: 1.25rem;
      font-weight: 800;
    }
    .brand-title span { color: var(--cyan); }
    .brand-sub {
      font-size: 0.72rem;
      font-weight: 700;
      color: var(--dim);
      letter-spacing: 1px;
    }
    .header-badges {
      display: flex;
      align-items: center;
      gap: 1rem;
    }
    .badge {
      display: inline-flex;
      align-items: center;
      gap: 0.5rem;
      padding: 0.35rem 0.85rem;
      border-radius: 9999px;
      font-size: 0.75rem;
      font-weight: 700;
      background: rgba(255, 255, 255, 0.04);
      border: 1px solid var(--border);
    }
    .badge-green { color: var(--green); border-color: rgba(16, 185, 129, 0.4); background: rgba(16, 185, 129, 0.1); }
    .badge-cyan { color: var(--cyan); border-color: rgba(0, 242, 254, 0.4); background: rgba(0, 242, 254, 0.1); }
    .dot { width: 8px; height: 8px; border-radius: 50%; background: var(--green); box-shadow: 0 0 8px var(--green); }

    .main {
      max-width: 1300px;
      margin: 2rem auto;
      padding: 0 2rem;
      display: flex;
      flex-direction: column;
      gap: 2.5rem;
    }

    .glass-card {
      background: var(--card);
      backdrop-filter: blur(20px);
      border: 1px solid var(--border);
      border-radius: 16px;
      padding: 1.75rem;
      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5);
    }

    /* Pipeline Step Cards */
    .steps-section h2 {
      font-size: 1.3rem;
      font-weight: 800;
      margin-bottom: 0.5rem;
      display: flex;
      align-items: center;
      gap: 0.5rem;
    }
    .steps-section .sub {
      font-size: 0.85rem;
      color: var(--muted);
      margin-bottom: 1.5rem;
    }

    .pipeline-grid {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 1.25rem;
    }
    .step-card {
      background: rgba(255, 255, 255, 0.02);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 1.25rem;
      display: flex;
      flex-direction: column;
      gap: 0.75rem;
      position: relative;
    }
    .step-number {
      font-size: 0.7rem;
      font-weight: 800;
      color: var(--cyan);
      background: rgba(0, 242, 254, 0.12);
      padding: 0.2rem 0.6rem;
      border-radius: 6px;
      width: fit-content;
      letter-spacing: 1px;
    }
    .step-title {
      font-size: 0.95rem;
      font-weight: 700;
      color: #fff;
    }
    .step-desc {
      font-size: 0.8rem;
      color: var(--muted);
      line-height: 1.4;
    }
    .step-code {
      font-family: 'JetBrains Mono', monospace;
      font-size: 0.72rem;
      background: rgba(0, 0, 0, 0.4);
      padding: 0.5rem 0.75rem;
      border-radius: 6px;
      color: #38bdf8;
      overflow-x: auto;
      white-space: pre-wrap;
      word-break: break-all;
    }

    /* Interactive Data Section */
    .split-grid {
      display: grid;
      grid-template-columns: 360px 1fr;
      gap: 1.75rem;
    }

    .form-title {
      font-size: 1.1rem;
      font-weight: 700;
      margin-bottom: 1rem;
      display: flex;
      align-items: center;
      gap: 0.5rem;
    }
    .form-group {
      display: flex;
      flex-direction: column;
      gap: 0.4rem;
      margin-bottom: 1rem;
    }
    .form-group label {
      font-size: 0.8rem;
      font-weight: 600;
      color: var(--muted);
    }
    .form-group input, .form-group select {
      background: rgba(255, 255, 255, 0.04);
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 0.6rem 0.85rem;
      color: #fff;
      font-size: 0.85rem;
      outline: none;
    }
    .form-group input:focus, .form-group select:focus {
      border-color: var(--cyan);
    }

    .btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 0.5rem;
      padding: 0.65rem 1.25rem;
      border-radius: 8px;
      font-size: 0.85rem;
      font-weight: 700;
      cursor: pointer;
      border: none;
      transition: all 0.2s;
    }
    .btn-primary {
      background: linear-gradient(135deg, var(--cyan), var(--blue));
      color: #070a12;
      width: 100%;
    }
    .btn-primary:hover {
      box-shadow: 0 4px 15px rgba(0, 242, 254, 0.4);
    }
    .btn-del {
      background: rgba(239, 68, 68, 0.15);
      color: var(--red);
      border: 1px solid rgba(239, 68, 68, 0.3);
      padding: 0.25rem 0.6rem;
      font-size: 0.72rem;
      border-radius: 6px;
    }
    .btn-del:hover { background: rgba(239, 68, 68, 0.3); }

    /* Table */
    .table-wrap {
      overflow-x: auto;
    }
    table {
      width: 100%;
      border-collapse: collapse;
      font-size: 0.85rem;
      text-align: left;
    }
    th {
      padding: 0.85rem 1rem;
      color: var(--dim);
      font-size: 0.72rem;
      font-weight: 700;
      letter-spacing: 1px;
      border-bottom: 1px solid var(--border);
    }
    td {
      padding: 0.85rem 1rem;
      border-bottom: 1px solid rgba(255, 255, 255, 0.04);
    }
    tr:hover td { background: rgba(255, 255, 255, 0.02); }
    .price-tag {
      font-family: 'JetBrains Mono', monospace;
      font-weight: 700;
      color: var(--green);
    }

    .alert {
      background: rgba(16, 185, 129, 0.15);
      border: 1px solid rgba(16, 185, 129, 0.3);
      color: var(--green);
      padding: 0.75rem 1rem;
      border-radius: 8px;
      font-size: 0.85rem;
      margin-bottom: 1.25rem;
    }

    .persistence-callout {
      border-left: 4px solid var(--amber);
      background: rgba(245, 158, 11, 0.08);
      padding: 1.25rem 1.5rem;
      border-radius: 8px;
      display: flex;
      flex-direction: column;
      gap: 0.5rem;
    }
    .persistence-callout h4 {
      color: var(--amber);
      font-size: 0.95rem;
    }
    .persistence-callout p {
      font-size: 0.85rem;
      color: var(--muted);
    }

    @media (max-width: 1024px) {
      .pipeline-grid { grid-template-columns: 1fr 1fr; }
      .split-grid { grid-template-columns: 1fr; }
    }
    @media (max-width: 640px) {
      .pipeline-grid { grid-template-columns: 1fr; }
      .header { flex-direction: column; gap: 1rem; align-items: flex-start; }
    }
  </style>
</head>
<body>

  <!-- Top Header Bar -->
  <header class="header">
    <div class="brand">
      <div class="brand-icon">⚡</div>
      <div>
        <div class="brand-title">SANDBOX<span>ENGINE</span> DEMO</div>
        <div class="brand-sub">FULL-STACK WEB &middot; PHP &middot; DATABASE INTERCONNECT</div>
      </div>
    </div>
    <div class="header-badges">
      <div class="badge badge-green"><span class="dot"></span> Container Online (:8080)</div>
      <div class="badge badge-cyan">⚡ Query Latency: <?= $query_time ?> ms</div>
      <div class="badge"> Apple Silicon Darwin</div>
    </div>
  </header>

  <main class="main">

    <!-- Section 1: The 4-Step Educational Pipeline -->
    <section class="steps-section">
      <h2>🚀 How This Container Operates (Step-by-Step Architecture)</h2>
      <p class="sub">Notice how this webpage running inside a Nocker container communicates with your Mac seamlessly:</p>

      <div class="pipeline-grid">
        <!-- Step 1 -->
        <div class="step-card">
          <span class="step-number">STEP 1: LAUNCH</span>
          <div class="step-title">One CLI Command</div>
          <div class="step-desc">Container started via Docker CLI using the Apple Silicon bare-metal daemon.</div>
          <div class="step-code">docker run -d -p 8080:8080 -v ...</div>
        </div>

        <!-- Step 2 -->
        <div class="step-card">
          <span class="step-number">STEP 2: VOLUME</span>
          <div class="step-title">APFS CoW Mount</div>
          <div class="step-desc">Web code (<code>/www</code>) and database (<code>/data</code>) sync directly from Mac host in 0.0ms.</div>
          <div class="step-code">-v ./www:/app -v ./data:/data</div>
        </div>

        <!-- Step 3 -->
        <div class="step-card">
          <span class="step-number">STEP 3: RUNTIME</span>
          <div class="step-title">PHP 8 + SQLite</div>
          <div class="step-desc">Executed inside container rootfs, querying <code>/data/silicon_store.db</code> at 3,000+ MB/s.</div>
          <div class="step-code">PDO::query("SELECT * FROM products")</div>
        </div>

        <!-- Step 4 -->
        <div class="step-card">
          <span class="step-number">STEP 4: NETWORK</span>
          <div class="step-title">Seamless Port Interconnect</div>
          <div class="step-desc">Host Mac connects to <code>localhost:8080</code> directly with zero VM bridge latency.</div>
          <div class="step-code">http://localhost:8080</div>
        </div>
      </div>
    </section>

    <?php if (!empty($action_msg)): ?>
      <div class="alert"><?= htmlspecialchars($action_msg) ?></div>
    <?php endif; ?>

    <!-- Section 2: Interactive Database Explorer -->
    <section class="glass-card">
      <div class="split-grid">
        
        <!-- Left: Insert Form -->
        <div class="form-panel">
          <div class="form-title">
            <span>📦</span>
            <span>Insert New Product</span>
          </div>
          <form method="POST">
            <input type="hidden" name="action" value="add">
            
            <div class="form-group">
              <label for="name">Product Name</label>
              <input type="text" id="name" name="name" placeholder="e.g. iPad Pro M4, Studio Mic" required autocomplete="off">
            </div>

            <div class="form-group">
              <label for="category">Category</label>
              <select id="category" name="category">
                <option value="Hardware">Hardware Silicon</option>
                <option value="Software">Software Tool</option>
                <option value="DevOps">DevOps / Container</option>
                <option value="Peripherals">Peripherals</option>
              </select>
            </div>

            <div class="form-group">
              <label for="price">Price (THB / USD)</label>
              <input type="number" step="0.01" id="price" name="price" value="29900.00" required>
            </div>

            <div class="form-group">
              <label for="stock">Initial Stock</label>
              <input type="number" id="stock" name="stock" value="10" required>
            </div>

            <button type="submit" class="btn btn-primary">
              <span>⚡ Insert into Container DB</span>
            </button>
          </form>
        </div>

        <!-- Right: Real-time Products Table -->
        <div class="table-panel">
          <div class="form-title" style="justify-content: space-between;">
            <div style="display:flex; align-items:center; gap:0.5rem;">
              <span>🗄️</span>
              <span>Live Database Records (<code>silicon_store.db</code>)</span>
            </div>
            <span style="font-size:0.75rem; color:var(--cyan); font-weight:700;"><?= count($products) ?> ITEMS FOUND</span>
          </div>

          <div class="table-wrap">
            <table>
              <thead>
                <tr>
                  <th>ID</th>
                  <th>NAME</th>
                  <th>CATEGORY</th>
                  <th>PRICE</th>
                  <th>STOCK</th>
                  <th style="text-align:right;">ACTION</th>
                </tr>
              </thead>
              <tbody>
                <?php foreach ($products as $p): ?>
                  <tr>
                    <td style="font-family:'JetBrains Mono'; color:var(--cyan);">#<?= htmlspecialchars($p['id']) ?></td>
                    <td><strong style="color:#fff;"><?= htmlspecialchars($p['name']) ?></strong></td>
                    <td><span class="badge"><?= htmlspecialchars($p['category']) ?></span></td>
                    <td class="price-tag"><?= number_format($p['price'], 2) ?></td>
                    <td><?= htmlspecialchars($p['stock']) ?> units</td>
                    <td style="text-align:right;">
                      <form method="POST" style="display:inline;" onsubmit="return confirm('Delete item #<?= $p['id'] ?>?');">
                        <input type="hidden" name="action" value="delete">
                        <input type="hidden" name="id" value="<?= $p['id'] ?>">
                        <button type="submit" class="btn-del">Delete</button>
                      </form>
                    </td>
                  </tr>
                <?php endforeach; ?>
              </tbody>
            </table>
          </div>
        </div>

      </div>
    </section>

    <!-- Section 3: The Ultimate Persistence Test Callout -->
    <section class="persistence-callout glass-card">
      <h4>🛡️ Data Persistence &amp; Container Destruction Challenge</h4>
      <p>
        Notice that all products are saved in <code>./data/silicon_store.db</code> mounted via <code>-v</code>. 
        You can destroy this container right now with <code>docker rm -f silicon-web-demo</code>, then re-launch it. 
        When you refresh this page, <strong>every product and table record will still be here 100%</strong> because APFS directly safeguards your host files!
      </p>
    </section>

  </main>

</body>
</html>
