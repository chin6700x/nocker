<?php
/**
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Multi-Language Parity Benchmark - PHP 8.2 Engine
 */

enum TaskStatus: string {
    case Pending = 'pending';
    case Running = 'running';
    case Done = 'done';
}

final readonly class JobPayload {
    public function __construct(
        public int $id,
        public string $taskName,
        public TaskStatus $status
    ) {}

    public function describe(): string {
        $badge = match ($this->status) {
            TaskStatus::Pending => '⏳ QUEUED',
            TaskStatus::Running => '⚡ RUNNING',
            TaskStatus::Done    => '✅ COMPLETED',
        };
        return "[Job #{$this->id}] {$this->taskName} => {$badge}";
    }
}

$job = new JobPayload(42, "APFS-CoW-Benchmark", TaskStatus::Done);
echo "⚡ [PHP-Runner] " . $job->describe() . PHP_EOL;
