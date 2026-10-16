// Author: Wirot Chookeaw (chin6700x)
// Copyright: (c) 2026 Wirot Chookeaw.
// License: Apache License 2.0
//
// ⚡ nocker: Multi-Language Parity Benchmark - Go Engine

package main

import (
	"fmt"
	"sync"
)

type WorkItem struct {
	ID    int
	Value int64
}

func squareWorker(in <-chan WorkItem, out chan<- WorkItem, wg *sync.WaitGroup) {
	defer wg.Done()
	for item := range in {
		out <- WorkItem{ID: item.ID, Value: item.Value * item.Value}
	}
}

func main() {
	inChan := make(chan WorkItem, 5)
	outChan := make(chan WorkItem, 5)
	var wg sync.WaitGroup

	wg.Add(1)
	go squareWorker(inChan, outChan, &wg)

	inChan <- WorkItem{ID: 1, Value: 12}
	close(inChan)

	wg.Wait()
	close(outChan)

	for res := range outChan {
		fmt.Printf("⚡ [Go-Runner] Worker Result ID=%d Squared=%d\n", res.ID, res.Value)
	}
}
