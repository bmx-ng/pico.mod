SuperStrict

Framework BRL.StandardIO
Import Collections.ArrayList
Import Collections.HashMap
Import Collections.HashSet
Import Collections.LinkedHashMap
Import Collections.LinkedList
Import Collections.PtrMap
Import Collections.Queue
Import Collections.Stack
Import Collections.StringMap
Import Collections.TreeMap
Import Collections.TreeSet

Type TValue
	Field number:Int

	Method New(number:Int)
		Self.number = number
	End Method
End Type

Local passed:Int = True

Local arrayList:TArrayList<String> = New TArrayList<String>
arrayList.Add("one")
arrayList.Add("two")
passed :& arrayList.Count() = 2 And arrayList[1] = "two"

Local hashMap:THashMap<String, TValue> = New THashMap<String, TValue>
hashMap.Put("answer", New TValue(42))
Local hashValue:TValue
passed :& hashMap.TryGetValue("answer", hashValue) And hashValue.number = 42

Local hashSet:THashSet<Int> = New THashSet<Int>
hashSet.Add(7)
passed :& hashSet.Contains(7)

Local linkedMap:TLinkedHashMap<String, Int> = New TLinkedHashMap<String, Int>
linkedMap.Put("first", 1)
passed :& linkedMap.ContainsKey("first")

Local linkedList:TLinkedList<Int> = New TLinkedList<Int>
linkedList.Add(3)
passed :& linkedList.Count() = 1

Local queue:TQueue<String> = New TQueue<String>
queue.Enqueue("queued")
passed :& queue.Dequeue() = "queued"

Local stack:TStack<Int> = New TStack<Int>
stack.Push(9)
passed :& stack.Pop() = 9

Local treeMap:TTreeMap<String, Int> = New TTreeMap<String, Int>
treeMap.Put("second", 2)
treeMap.Put("first", 1)
passed :& treeMap.ContainsKey("first") And treeMap["second"] = 2

Local treeSet:TTreeSet<String> = New TTreeSet<String>
treeSet.Add("second")
treeSet.Add("first")
passed :& treeSet.Contains("first") And treeSet.Count() = 2

Local stringMap:TStringMap = New TStringMap(False)
stringMap.Insert("Mixed", New TValue(11))
passed :& stringMap.Contains("mixed")

Local pointerMap:TPtrMap = New TPtrMap
Local key:Byte Ptr = MemAlloc(4)
pointerMap.Insert(key, New TValue(12))
passed :& pointerMap.Contains(key)
MemFree(key)

Print "generic collections: " + passed
